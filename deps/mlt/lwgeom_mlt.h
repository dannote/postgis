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

#ifndef LWGEOM_MLT_H
#define LWGEOM_MLT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "liblwgeom.h"
#include "lwgeom_log.h"

/*
 * Opaque handle to the MLT encoder context (C++ internals hidden).
 * Wraps mlt::Encoder, accumulates features across transfn calls,
 * and produces the final binary tile in finalfn.
 */
typedef struct mlt_agg_context mlt_agg_context;

/*
 * Property value that can be passed from the PostgreSQL aggregate
 * into the encoder without exposing any C++ types.
 */
enum mlt_prop_type {
	MLT_PROP_BOOL,
	MLT_PROP_INT32,
	MLT_PROP_INT64,
	MLT_PROP_FLOAT,
	MLT_PROP_DOUBLE,
	MLT_PROP_STRING
};

typedef struct mlt_property {
	const char *key;
	enum mlt_prop_type type;
	union {
		int bool_val;
		int32_t int32_val;
		int64_t int64_val;
		float float_val;
		double double_val;
		const char *string_val;
	} val;
} mlt_property;

mlt_agg_context *mlt_agg_create(const char *layer_name, uint32_t extent);
void mlt_agg_destroy(mlt_agg_context *ctx);

void mlt_agg_add_feature(mlt_agg_context *ctx,
                         uint64_t id,
                         const LWGEOM *lwgeom,
                         const mlt_property *properties,
                         uint32_t n_properties);

/*
 * Encode accumulated features into an MLT binary blob.
 * Caller must lwfree() the returned buffer.
 * Sets *out_size to the size of the returned buffer.
 */
uint8_t *mlt_agg_encode(mlt_agg_context *ctx, size_t *out_size);

/*
 * Serialise / deserialise the accumulated feature state.
 * Used for PostgreSQL parallel aggregate support.
 * Caller must lwfree() the returned buffers.
 */
uint8_t *mlt_agg_serialize(mlt_agg_context *ctx, size_t *out_size);
mlt_agg_context *mlt_agg_deserialize(const uint8_t *data, size_t size);
mlt_agg_context *mlt_agg_combine(mlt_agg_context *ctx1, mlt_agg_context *ctx2);

#ifdef __cplusplus
}
#endif

#endif /* LWGEOM_MLT_H */
