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

#ifndef MLT_H_
#define MLT_H_ 1

#include <stdlib.h>
#include "postgres.h"
#include "utils/builtins.h"
#include "utils/array.h"
#include "utils/typcache.h"
#include "utils/lsyscache.h"
#include "catalog/pg_type.h"
#include "catalog/namespace.h"
#include "executor/executor.h"
#include "access/htup_details.h"
#include "access/htup.h"
#include "../postgis_config.h"
#include "liblwgeom.h"
#include "lwgeom_pg.h"
#include "lwgeom_log.h"

#ifdef HAVE_LIBMLT

#include "../deps/mlt/lwgeom_mlt.h"

typedef struct mlt_column_cache {
	uint32_t *column_oid;
	char **column_names;
	Datum *values;
	bool *nulls;
	TupleDesc tupdesc;
} mlt_column_cache;

typedef struct mlt_pg_context {
	MemoryContext trans_context;
	char *name;
	uint32_t extent;

	char *id_name;
	uint32_t id_index;

	char *geom_name;
	uint32_t geom_index;

	HeapTupleHeader row;
	mlt_agg_context *encoder_ctx;

	mlt_column_cache column_cache;
} mlt_pg_context;

void mlt_pg_init_context(mlt_pg_context *ctx);
void mlt_pg_transfn(mlt_pg_context *ctx);
bytea *mlt_pg_finalfn(mlt_pg_context *ctx);

#endif /* HAVE_LIBMLT */

#endif
