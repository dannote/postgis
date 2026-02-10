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

#include "postgres.h"
#include "utils/builtins.h"
#include "../postgis_config.h"
#include "lwgeom_pg.h"
#include "lwgeom_log.h"
#include "liblwgeom.h"
#include "mlt.h"

/**
 * Process input parameters and row data into state
 */
PG_FUNCTION_INFO_V1(pgis_asmlt_transfn);
Datum
pgis_asmlt_transfn(PG_FUNCTION_ARGS)
{
#ifndef HAVE_LIBMLT
	elog(ERROR, "ST_AsMLT: compiled without MLT support");
	PG_RETURN_NULL();
#else
	MemoryContext aggcontext, old_context;
	mlt_pg_context *ctx;

	postgis_initialize_cache();

	if (!AggCheckCallContext(fcinfo, &aggcontext))
		elog(ERROR, "%s called in non-aggregate context", __func__);

	if (PG_ARGISNULL(0))
	{
		old_context = MemoryContextSwitchTo(aggcontext);
		ctx = palloc(sizeof(*ctx));
		memset(ctx, 0, sizeof(*ctx));

		ctx->name = "default";
		if (PG_NARGS() > 2 && !PG_ARGISNULL(2))
			ctx->name = text_to_cstring(PG_GETARG_TEXT_P(2));
		ctx->extent = 4096;
		if (PG_NARGS() > 3 && !PG_ARGISNULL(3))
			ctx->extent = PG_GETARG_INT32(3);
		ctx->geom_name = NULL;
		if (PG_NARGS() > 4 && !PG_ARGISNULL(4))
			ctx->geom_name = text_to_cstring(PG_GETARG_TEXT_P(4));
		ctx->id_name = NULL;
		if (PG_NARGS() > 5 && !PG_ARGISNULL(5))
			ctx->id_name = text_to_cstring(PG_GETARG_TEXT_P(5));

		ctx->trans_context = AllocSetContextCreate(aggcontext, "MLT transfn", ALLOCSET_DEFAULT_SIZES);

		MemoryContextSwitchTo(ctx->trans_context);
		mlt_pg_init_context(ctx);
		MemoryContextSwitchTo(old_context);
	}
	else
	{
		ctx = (mlt_pg_context *)PG_GETARG_POINTER(0);
	}

	if (!type_is_rowtype(get_fn_expr_argtype(fcinfo->flinfo, 1)))
		elog(ERROR, "%s: parameter row cannot be other than a rowtype", __func__);
	ctx->row = PG_GETARG_HEAPTUPLEHEADER(1);

	old_context = MemoryContextSwitchTo(ctx->trans_context);
	mlt_pg_transfn(ctx);
	MemoryContextSwitchTo(old_context);

	PG_FREE_IF_COPY(ctx->row, 1);
	PG_RETURN_POINTER(ctx);
#endif
}

/**
 * Encode final state to MapLibre Tile
 */
PG_FUNCTION_INFO_V1(pgis_asmlt_finalfn);
Datum
pgis_asmlt_finalfn(PG_FUNCTION_ARGS)
{
#ifndef HAVE_LIBMLT
	elog(ERROR, "ST_AsMLT: compiled without MLT support");
	PG_RETURN_NULL();
#else
	mlt_pg_context *ctx;
	bytea *buf;

	if (!AggCheckCallContext(fcinfo, NULL))
		elog(ERROR, "%s called in non-aggregate context", __func__);

	if (PG_ARGISNULL(0))
	{
		bytea *emptybuf = palloc(VARHDRSZ);
		SET_VARSIZE(emptybuf, VARHDRSZ);
		PG_RETURN_BYTEA_P(emptybuf);
	}

	ctx = (mlt_pg_context *)PG_GETARG_POINTER(0);
	buf = mlt_pg_finalfn(ctx);

	if (ctx->encoder_ctx)
		mlt_agg_destroy(ctx->encoder_ctx);

	if (ctx->column_cache.tupdesc)
		ReleaseTupleDesc(ctx->column_cache.tupdesc);

	PG_RETURN_BYTEA_P(buf);
#endif
}

PG_FUNCTION_INFO_V1(pgis_asmlt_combinefn);
Datum
pgis_asmlt_combinefn(PG_FUNCTION_ARGS)
{
#ifndef HAVE_LIBMLT
	elog(ERROR, "ST_AsMLT: compiled without MLT support");
	PG_RETURN_NULL();
#else
	MemoryContext aggcontext, old_context;
	mlt_pg_context *ctx1, *ctx2;

	if (!AggCheckCallContext(fcinfo, &aggcontext))
		elog(ERROR, "%s called in non-aggregate context", __func__);

	ctx1 = PG_ARGISNULL(0) ? NULL : (mlt_pg_context *)PG_GETARG_POINTER(0);
	ctx2 = PG_ARGISNULL(1) ? NULL : (mlt_pg_context *)PG_GETARG_POINTER(1);

	if (!ctx1 && !ctx2)
		PG_RETURN_NULL();
	if (!ctx1)
		PG_RETURN_POINTER(ctx2);
	if (!ctx2)
		PG_RETURN_POINTER(ctx1);

	old_context = MemoryContextSwitchTo(aggcontext);
	ctx1->encoder_ctx = mlt_agg_combine(ctx1->encoder_ctx, ctx2->encoder_ctx);
	ctx2->encoder_ctx = NULL;
	MemoryContextSwitchTo(old_context);

	PG_RETURN_POINTER(ctx1);
#endif
}
