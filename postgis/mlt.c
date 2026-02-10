/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright (C) 2025 Dan Polevoy
 *
 **********************************************************************/

#include <stdbool.h>
#include <string.h>

#include "mlt.h"

#ifdef HAVE_LIBMLT

static TupleDesc
get_tuple_desc(mlt_pg_context *ctx)
{
	Oid tupType = HeapTupleHeaderGetTypeId(ctx->row);
	int32 tupTypmod = HeapTupleHeaderGetTypMod(ctx->row);
	return lookup_rowtype_tupdesc(tupType, tupTypmod);
}

static void
parse_column_keys(mlt_pg_context *ctx)
{
	uint32_t i, natts;
	bool geom_found = false;

	ctx->column_cache.tupdesc = get_tuple_desc(ctx);
	natts = ctx->column_cache.tupdesc->natts;

	ctx->column_cache.column_oid = palloc(sizeof(uint32_t) * natts);
	ctx->column_cache.column_names = palloc(sizeof(char *) * natts);
	ctx->column_cache.values = palloc(sizeof(Datum) * natts);
	ctx->column_cache.nulls = palloc(sizeof(bool) * natts);

	for (i = 0; i < natts; i++)
	{
		Oid typoid = getBaseType(TupleDescAttr(ctx->column_cache.tupdesc, i)->atttypid);
		char *tkey = TupleDescAttr(ctx->column_cache.tupdesc, i)->attname.data;

		ctx->column_cache.column_oid[i] = typoid;
		ctx->column_cache.column_names[i] = tkey;

		if (ctx->geom_name == NULL)
		{
			if (!geom_found && typoid == postgis_oid(GEOMETRYOID))
			{
				ctx->geom_index = i;
				geom_found = true;
				continue;
			}
		}
		else
		{
			if (!geom_found && strcmp(tkey, ctx->geom_name) == 0)
			{
				ctx->geom_index = i;
				geom_found = true;
				continue;
			}
		}

		if (ctx->id_name && (ctx->id_index == UINT32_MAX) && (strcmp(tkey, ctx->id_name) == 0) &&
		    (typoid == INT2OID || typoid == INT4OID || typoid == INT8OID))
		{
			ctx->id_index = i;
		}
	}

	if (!geom_found)
		elog(ERROR, "ST_AsMLT: no geometry column found");

	if (ctx->id_name != NULL && ctx->id_index == UINT32_MAX)
		elog(ERROR, "ST_AsMLT: could not find column '%s' of integer type", ctx->id_name);
}

void
mlt_pg_init_context(mlt_pg_context *ctx)
{
	if (ctx->extent == 0)
		elog(ERROR, "ST_AsMLT: extent cannot be 0");

	ctx->encoder_ctx = mlt_agg_create(ctx->name, ctx->extent);
	ctx->id_index = UINT32_MAX;
	ctx->geom_index = UINT32_MAX;
	memset(&ctx->column_cache, 0, sizeof(ctx->column_cache));
}

void
mlt_pg_transfn(mlt_pg_context *ctx)
{
	bool isnull = false;
	Datum datum;
	GSERIALIZED *gs;
	LWGEOM *lwgeom;
	uint64_t fid = 0;
	uint32_t i, natts;
	mlt_property *props = NULL;
	uint32_t n_props = 0;
	HeapTupleData tuple;

	if (ctx->geom_index == UINT32_MAX)
		parse_column_keys(ctx);

	datum = GetAttributeByNum(ctx->row, ctx->geom_index + 1, &isnull);
	if (isnull)
		return;

	gs = (GSERIALIZED *)PG_DETOAST_DATUM(datum);
	lwgeom = lwgeom_from_gserialized(gs);

	if (lwgeom_is_empty(lwgeom))
	{
		lwgeom_free(lwgeom);
		return;
	}

	/* Deform tuple to get all columns efficiently */
	natts = ctx->column_cache.tupdesc->natts;
	tuple.t_len = HeapTupleHeaderGetDatumLength(ctx->row);
	ItemPointerSetInvalid(&(tuple.t_self));
	tuple.t_tableOid = InvalidOid;
	tuple.t_data = ctx->row;
	heap_deform_tuple(&tuple, ctx->column_cache.tupdesc, ctx->column_cache.values, ctx->column_cache.nulls);

	/* Extract feature ID */
	if (ctx->id_index != UINT32_MAX && !ctx->column_cache.nulls[ctx->id_index])
	{
		Oid typoid = ctx->column_cache.column_oid[ctx->id_index];
		Datum id_datum = ctx->column_cache.values[ctx->id_index];
		switch (typoid)
		{
		case INT2OID:
			fid = (uint64_t)DatumGetInt16(id_datum);
			break;
		case INT4OID:
			fid = (uint64_t)DatumGetInt32(id_datum);
			break;
		case INT8OID:
			fid = (uint64_t)DatumGetInt64(id_datum);
			break;
		default:
			break;
		}
	}

	/* Build property array (skip geom and id columns) */
	props = palloc(sizeof(mlt_property) * natts);

	for (i = 0; i < natts; i++)
	{
		Oid typoid;
		Datum val;

		if (i == ctx->geom_index)
			continue;
		if (ctx->id_index != UINT32_MAX && i == ctx->id_index)
			continue;
		if (ctx->column_cache.nulls[i])
			continue;

		typoid = ctx->column_cache.column_oid[i];
		val = ctx->column_cache.values[i];

		props[n_props].key = ctx->column_cache.column_names[i];

		switch (typoid)
		{
		case BOOLOID:
			props[n_props].type = MLT_PROP_BOOL;
			props[n_props].val.bool_val = DatumGetBool(val);
			n_props++;
			break;
		case INT2OID:
			props[n_props].type = MLT_PROP_INT32;
			props[n_props].val.int32_val = (int32_t)DatumGetInt16(val);
			n_props++;
			break;
		case INT4OID:
			props[n_props].type = MLT_PROP_INT32;
			props[n_props].val.int32_val = DatumGetInt32(val);
			n_props++;
			break;
		case INT8OID:
			props[n_props].type = MLT_PROP_INT64;
			props[n_props].val.int64_val = DatumGetInt64(val);
			n_props++;
			break;
		case FLOAT4OID:
			props[n_props].type = MLT_PROP_FLOAT;
			props[n_props].val.float_val = DatumGetFloat4(val);
			n_props++;
			break;
		case FLOAT8OID:
			props[n_props].type = MLT_PROP_DOUBLE;
			props[n_props].val.double_val = DatumGetFloat8(val);
			n_props++;
			break;
		case TEXTOID:
			props[n_props].type = MLT_PROP_STRING;
			props[n_props].val.string_val = text_to_cstring(DatumGetTextP(val));
			n_props++;
			break;
		case CSTRINGOID:
			props[n_props].type = MLT_PROP_STRING;
			props[n_props].val.string_val = DatumGetCString(val);
			n_props++;
			break;
		default: {
			/* Fall back: render as string */
			Oid foutoid;
			bool typisvarlena;
			getTypeOutputInfo(typoid, &foutoid, &typisvarlena);
			props[n_props].type = MLT_PROP_STRING;
			props[n_props].val.string_val = OidOutputFunctionCall(foutoid, val);
			n_props++;
			break;
		}
		}
	}

	mlt_agg_add_feature(ctx->encoder_ctx, fid, lwgeom, props, n_props);

	lwgeom_free(lwgeom);
	pfree(props);
}

bytea *
mlt_pg_finalfn(mlt_pg_context *ctx)
{
	size_t len;
	uint8_t *encoded;
	bytea *result;

	if (!ctx || !ctx->encoder_ctx)
	{
		result = palloc(VARHDRSZ);
		SET_VARSIZE(result, VARHDRSZ);
		return result;
	}

	encoded = mlt_agg_encode(ctx->encoder_ctx, &len);

	if (len == 0)
	{
		result = palloc(VARHDRSZ);
		SET_VARSIZE(result, VARHDRSZ);
		if (encoded)
			lwfree(encoded);
		return result;
	}

	result = palloc(VARHDRSZ + len);
	memcpy(VARDATA(result), encoded, len);
	SET_VARSIZE(result, VARHDRSZ + len);

	lwfree(encoded);
	return result;
}

#endif /* HAVE_LIBMLT */
