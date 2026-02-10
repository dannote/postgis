-- ST_AsMLT regression tests

-- Geometry types: point
SELECT 'G1', length(ST_AsMLT(q, 'test', 4096, 'geom')) > 0 FROM (
	SELECT 1 AS id, ST_GeomFromText('POINT(25 17)') AS geom) q;

-- Geometry types: multipoint
SELECT 'G2', length(ST_AsMLT(q, 'test', 4096, 'geom')) > 0 FROM (
	SELECT 1 AS id, ST_GeomFromText('MULTIPOINT(25 17, 26 18)') AS geom) q;

-- Geometry types: linestring
SELECT 'G3', length(ST_AsMLT(q, 'test', 4096, 'geom')) > 0 FROM (
	SELECT 1 AS id, ST_GeomFromText('LINESTRING(0 0, 1000 1000)') AS geom) q;

-- Geometry types: multilinestring
SELECT 'G4', length(ST_AsMLT(q, 'test', 4096, 'geom')) > 0 FROM (
	SELECT 1 AS id, ST_GeomFromText('MULTILINESTRING((1 1, 501 501),(2 2, 502 502))') AS geom) q;

-- Geometry types: polygon
SELECT 'G5', length(ST_AsMLT(q, 'test', 4096, 'geom')) > 0 FROM (
	SELECT 1 AS id, ST_GeomFromText('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))') AS geom) q;

-- Geometry types: polygon with hole
SELECT 'G6', length(ST_AsMLT(q, 'test', 4096, 'geom')) > 0 FROM (
	SELECT 1 AS id, ST_GeomFromText('POLYGON((0 0, 100 0, 100 100, 0 100, 0 0),(20 20, 80 20, 80 80, 20 80, 20 20))') AS geom) q;

-- Geometry types: multipolygon
SELECT 'G7', length(ST_AsMLT(q, 'test', 4096, 'geom')) > 0 FROM (
	SELECT 1 AS id, ST_Multi(ST_Union(
		ST_Buffer(ST_Point(0, 0), 5),
		ST_Buffer(ST_Point(20, 20), 5)
	)) AS geom) q;

-- Property types: integer
SELECT 'P1', length(ST_AsMLT(q, 'test', 4096, 'geom')) > 0 FROM (
	SELECT 1 AS id, 42 AS int_col, ST_Point(0, 0) AS geom) q;

-- Property types: bigint
SELECT 'P2', length(ST_AsMLT(q, 'test', 4096, 'geom')) > 0 FROM (
	SELECT 1 AS id, 9999999999::bigint AS big_col, ST_Point(0, 0) AS geom) q;

-- Property types: float
SELECT 'P3', length(ST_AsMLT(q, 'test', 4096, 'geom')) > 0 FROM (
	SELECT 1 AS id, 3.14::float4 AS float_col, ST_Point(0, 0) AS geom) q;

-- Property types: double
SELECT 'P4', length(ST_AsMLT(q, 'test', 4096, 'geom')) > 0 FROM (
	SELECT 1 AS id, 3.14159265358979::float8 AS double_col, ST_Point(0, 0) AS geom) q;

-- Property types: text
SELECT 'P5', length(ST_AsMLT(q, 'test', 4096, 'geom')) > 0 FROM (
	SELECT 1 AS id, 'hello world'::text AS text_col, ST_Point(0, 0) AS geom) q;

-- Property types: boolean
SELECT 'P6', length(ST_AsMLT(q, 'test', 4096, 'geom')) > 0 FROM (
	SELECT 1 AS id, true AS bool_col, ST_Point(0, 0) AS geom) q;

-- Property types: mixed
SELECT 'P7', length(ST_AsMLT(q, 'test', 4096, 'geom')) > 0 FROM (
	SELECT 1 AS id, 42 AS int_col, 'text'::text AS str_col, 3.14::float8 AS dbl_col, true AS bool_col,
	       ST_Point(0, 0) AS geom) q;

-- Multiple features
SELECT 'M1', length(ST_AsMLT(q, 'test', 4096, 'geom')) > 0 FROM (
	SELECT id, ST_Point(id, id) AS geom FROM generate_series(1, 100) AS id) q;

-- Multiple features with properties
SELECT 'M2', length(ST_AsMLT(q, 'lands', 4096, 'geom')) > 0 FROM (
	SELECT i AS id, ST_Buffer(ST_Point(i, i), 1) AS geom, 'cat_' || (i % 5) AS category
	FROM generate_series(1, 50) AS i) q;

-- MLT smaller than MVT
SELECT 'S1',
	length(ST_AsMLT(q, 'test', 4096, 'geom')) < length(ST_AsMVT(q, 'test', 4096, 'geom'))
FROM (
	SELECT i AS id, ST_Buffer(ST_Point(random()*100, random()*100), 2) AS geom,
	       'cat_' || (i % 7) AS category, i * 100 AS area
	FROM generate_series(1, 500) AS i) q;

-- NULL geometry rows skipped
SELECT 'N1', length(ST_AsMLT(q, 'test', 4096, 'geom')) > 0 FROM (
	SELECT 1 AS id, ST_Point(0, 0) AS geom
	UNION ALL SELECT 2, NULL::geometry
	UNION ALL SELECT 3, ST_Point(1, 1)) q;

-- Empty result returns empty bytea
SELECT 'N2', length(ST_AsMLT(q, 'test', 4096, 'geom')) FROM (
	SELECT 1 AS id, ST_Point(0, 0) AS geom WHERE false) q;

-- Overload: 1 arg (just row)
SELECT 'O1', length(ST_AsMLT(q)) > 0 FROM (
	SELECT 1 AS id, ST_Point(0, 0) AS geom) q;

-- Overload: 2 args (row, layer name)
SELECT 'O2', length(ST_AsMLT(q, 'mylayer')) > 0 FROM (
	SELECT 1 AS id, ST_Point(0, 0) AS geom) q;

-- Overload: 3 args (row, layer name, extent)
SELECT 'O3', length(ST_AsMLT(q, 'test', 256)) > 0 FROM (
	SELECT 1 AS id, ST_Point(0, 0) AS geom) q;

-- Overload: 4 args (row, layer name, extent, geom column)
SELECT 'O4', length(ST_AsMLT(q, 'test', 4096, 'g')) > 0 FROM (
	SELECT 1 AS id, ST_Point(0, 0) AS g) q;

-- Overload: 5 args (row, layer name, extent, geom column, id column)
SELECT 'O5', length(ST_AsMLT(q, 'test', 4096, 'geom', 'id')) > 0 FROM (
	SELECT 1 AS id, ST_Point(0, 0) AS geom) q;

-- Deterministic: same input produces same output
SELECT 'D1',
	ST_AsMLT(q, 'test', 4096, 'geom') = ST_AsMLT(q, 'test', 4096, 'geom')
FROM (SELECT 1 AS id, ST_Point(5, 5) AS geom) q;
