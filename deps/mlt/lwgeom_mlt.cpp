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

#include "lwgeom_mlt.h"

#include <mlt/encoder.hpp>

#include <cstring>
#include <vector>
#include <string>

using Vertex = mlt::Encoder::Vertex;
using GeometryType = mlt::Encoder::GeometryType;
using Feature = mlt::Encoder::Feature;
using Layer = mlt::Encoder::Layer;
using PropertyValue = mlt::Encoder::PropertyValue;

struct mlt_agg_context {
	std::string layer_name;
	uint32_t extent;
	std::vector<Feature> features;
};

/* ----------------------------------------------------------------
 * Geometry conversion: LWGEOM → mlt::Encoder::Geometry
 * Mirrors the MVT pattern but targets the MLT Geometry struct.
 * ---------------------------------------------------------------- */

static Vertex
vertex_from_point2d(const POINT2D *p)
{
	return {static_cast<int32_t>(p->x), static_cast<int32_t>(p->y)};
}

static std::vector<Vertex>
vertices_from_pa(const POINTARRAY *pa)
{
	std::vector<Vertex> verts;
	verts.reserve(pa->npoints);
	for (uint32_t i = 0; i < pa->npoints; i++)
		verts.push_back(vertex_from_point2d(getPoint2d_cp(pa, i)));
	return verts;
}

static mlt::Encoder::Geometry
convert_point(const LWPOINT *point)
{
	mlt::Encoder::Geometry g;
	g.type = GeometryType::POINT;
	g.coordinates.push_back(vertex_from_point2d(getPoint2d_cp(point->point, 0)));
	return g;
}

static mlt::Encoder::Geometry
convert_mpoint(const LWMPOINT *mpoint)
{
	mlt::Encoder::Geometry g;
	g.type = GeometryType::MULTIPOINT;
	for (uint32_t i = 0; i < mpoint->ngeoms; i++)
		g.coordinates.push_back(vertex_from_point2d(getPoint2d_cp(mpoint->geoms[i]->point, 0)));
	return g;
}

static mlt::Encoder::Geometry
convert_line(const LWLINE *line)
{
	mlt::Encoder::Geometry g;
	g.type = GeometryType::LINESTRING;
	g.coordinates = vertices_from_pa(line->points);
	return g;
}

static mlt::Encoder::Geometry
convert_mline(const LWMLINE *mline)
{
	mlt::Encoder::Geometry g;
	g.type = GeometryType::MULTILINESTRING;
	for (uint32_t i = 0; i < mline->ngeoms; i++)
		g.parts.push_back(vertices_from_pa(mline->geoms[i]->points));
	return g;
}

static mlt::Encoder::Geometry
convert_poly(const LWPOLY *poly)
{
	mlt::Encoder::Geometry g;
	g.type = GeometryType::POLYGON;
	for (uint32_t i = 0; i < poly->nrings; i++)
	{
		auto ring = vertices_from_pa(poly->rings[i]);
		g.coordinates.insert(g.coordinates.end(), ring.begin(), ring.end());
		g.ringSizes.push_back(poly->rings[i]->npoints);
	}
	return g;
}

static mlt::Encoder::Geometry
convert_mpoly(const LWMPOLY *mpoly)
{
	mlt::Encoder::Geometry g;
	g.type = GeometryType::MULTIPOLYGON;
	for (uint32_t i = 0; i < mpoly->ngeoms; i++)
	{
		const LWPOLY *poly = mpoly->geoms[i];
		std::vector<Vertex> poly_verts;
		std::vector<uint32_t> ring_sizes;
		for (uint32_t r = 0; r < poly->nrings; r++)
		{
			auto ring = vertices_from_pa(poly->rings[r]);
			poly_verts.insert(poly_verts.end(), ring.begin(), ring.end());
			ring_sizes.push_back(poly->rings[r]->npoints);
		}
		g.parts.push_back(std::move(poly_verts));
		g.partRingSizes.push_back(std::move(ring_sizes));
	}
	return g;
}

static mlt::Encoder::Geometry
convert_lwgeom(const LWGEOM *lwgeom)
{
	switch (lwgeom->type)
	{
	case POINTTYPE:
		return convert_point(lwgeom_as_lwpoint(lwgeom));
	case LINETYPE:
		return convert_line(lwgeom_as_lwline(lwgeom));
	case POLYGONTYPE:
		return convert_poly(lwgeom_as_lwpoly(lwgeom));
	case MULTIPOINTTYPE:
		return convert_mpoint(lwgeom_as_lwmpoint(lwgeom));
	case MULTILINETYPE:
		return convert_mline(lwgeom_as_lwmline(lwgeom));
	case MULTIPOLYGONTYPE:
		return convert_mpoly(lwgeom_as_lwmpoly(lwgeom));
	default:
		lwerror("mlt_agg_add_feature: geometry type '%s' not supported", lwtype_name(lwgeom->type));
		/* not reached, lwerror longjmps */
		return {};
	}
}

/* ----------------------------------------------------------------
 * Public C API
 * ---------------------------------------------------------------- */

extern "C" {

mlt_agg_context *
mlt_agg_create(const char *layer_name, uint32_t extent)
{
	auto *ctx = new mlt_agg_context();
	ctx->layer_name = layer_name;
	ctx->extent = extent;
	return ctx;
}

void
mlt_agg_destroy(mlt_agg_context *ctx)
{
	delete ctx;
}

void
mlt_agg_add_feature(mlt_agg_context *ctx,
		    uint64_t id,
		    const LWGEOM *lwgeom,
		    const mlt_property *properties,
		    uint32_t n_properties)
{
	Feature f;
	f.id = id;
	f.geometry = convert_lwgeom(lwgeom);

	for (uint32_t i = 0; i < n_properties; i++)
	{
		const auto &p = properties[i];
		switch (p.type)
		{
		case MLT_PROP_BOOL:
			f.properties[p.key] = static_cast<bool>(p.val.bool_val);
			break;
		case MLT_PROP_INT32:
			f.properties[p.key] = p.val.int32_val;
			break;
		case MLT_PROP_INT64:
			f.properties[p.key] = p.val.int64_val;
			break;
		case MLT_PROP_FLOAT:
			f.properties[p.key] = p.val.float_val;
			break;
		case MLT_PROP_DOUBLE:
			f.properties[p.key] = p.val.double_val;
			break;
		case MLT_PROP_STRING:
			f.properties[p.key] = std::string(p.val.string_val);
			break;
		}
	}

	ctx->features.push_back(std::move(f));
}

uint8_t *
mlt_agg_encode(mlt_agg_context *ctx, size_t *out_size)
{
	mlt::Encoder encoder;
	mlt::EncoderConfig config;

	Layer layer;
	layer.name = ctx->layer_name;
	layer.extent = ctx->extent;
	layer.features = std::move(ctx->features);

	auto encoded = encoder.encode({layer}, config);

	*out_size = encoded.size();
	auto *buf = static_cast<uint8_t *>(lwalloc(encoded.size()));
	std::memcpy(buf, encoded.data(), encoded.size());
	return buf;
}

uint8_t *
mlt_agg_serialize(mlt_agg_context *ctx, size_t *out_size)
{
	/* For parallel aggregation we encode the current state into an MLT
	 * blob, same as finalfn.  The combinefn will merge by concatenating
	 * feature lists from deserialized tiles. */
	return mlt_agg_encode(ctx, out_size);
}

mlt_agg_context *
mlt_agg_deserialize(const uint8_t *data, size_t size)
{
	/*
	 * Parallel aggregate deserialize: we receive an MLT blob that was
	 * produced by mlt_agg_serialize (which is just mlt_agg_encode).
	 *
	 * Rather than decoding back to features we keep the raw blob and
	 * treat combine as a no-op concatenation.  However, the MLT format
	 * doesn't trivially support concatenation of two independent tiles,
	 * so we take the simple path: create a fresh context with the blob
	 * stored as already-encoded.  combine will just re-encode both.
	 *
	 * For the initial implementation we skip true parallel support and
	 * just re-create a context.  PostgreSQL will not use the parallel
	 * path unless we declare the aggregate as PARALLEL SAFE and provide
	 * these functions — so providing stubs is sufficient.
	 */
	(void)data;
	(void)size;
	lwerror("mlt_agg_deserialize: parallel MLT aggregation not yet implemented");
	return NULL;
}

mlt_agg_context *
mlt_agg_combine(mlt_agg_context *ctx1, mlt_agg_context *ctx2)
{
	if (!ctx1)
		return ctx2;
	if (!ctx2)
		return ctx1;

	ctx1->features.insert(ctx1->features.end(),
			      std::make_move_iterator(ctx2->features.begin()),
			      std::make_move_iterator(ctx2->features.end()));
	delete ctx2;
	return ctx1;
}

} /* extern "C" */
