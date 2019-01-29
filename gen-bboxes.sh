#!/bin/bash -e

# create postcode district (BN1, SW1A, etc) bounding boxes
# suitable for reverse lookup (location -> postcode)

# run this before gen-structs.rb if you need reverse lookup capability

# requirements:
# CodePoint Open + BoundaryLine data
# xsv (on Mac: brew install xsv)
# Postgres / PostGIS (on Mac: get Postgres.app)

# remember to start Postgres and export appropriate PATH for Postgres binaries, then run: 
# ./gen-bboxes.sh /path/to/codepoint-open/csv/files /path/to/boundaryline/shapefiles

# expect to wait 30 - 60 mins


CPODATADIR="$1"
BLDATADIR="$2"

echo "Creating database ..."

createdb codepointopen

echo '
create extension postgis;

create table cpo 
( pc text
, e integer
, n integer
);
' | psql -d codepointopen

echo "Loading data ..."

# the following pipeline:
# - concatenates all CSV data
# - filters out rows with quality indicator 90 (quality is col 2, 90 means no coordinates)
# - picks out postcode, easting and northing (cols 1, 3, 4)
# - inserts into table cpo

cat ${CPODATADIR}/*.csv | \
  xsv search --no-headers --invert-match --select 2 90 | \
  xsv select 1,3,4 | \
  psql -d codepointopen -c '\copy cpo from stdin csv'

shp2pgsql -D -s 27700 "${BLDATADIR}/european_region_region.shp" euregions | psql -d codepointopen

echo "Generating Voronoi polygons ..."

echo '
alter table cpo add column pt geometry(Point, 27700); 
update cpo set pt = st_setsrid(st_makepoint(e, n), 27700);
create index pt_idx on cpo using gist(pt);  -- not clear if this speeds up st_voronoipolygons or not

alter table cpo add column outward text;
update cpo set outward = rtrim(left(pc, -3));
create index outward_idx on cpo(outward);
analyze cpo;

create table cpo_vor as (
  select (st_dump(st_voronoipolygons(st_collect(pt)))).geom as geom from cpo
);
create index vor_idx on cpo_vor using gist(geom);
analyze cpo_vor;
' | psql -d codepointopen

echo "Creating GB outline ..."

echo '
create table gbsimple as (
  select st_makevalid(st_simplifypreservetopology(st_union(st_buffer(geom, 500)), 250)) as geom from euregions
);  -- takes 15m (st_makevalid should not be necessary, but it is)
' | psql -d codepointopen

echo "Clipping Voronoi polygons to GB outline ..."

echo '
create table cpo_clipped as (
  select case when st_within(c.geom, g.geom) then c.geom else st_intersection(c.geom, g.geom) end as geom 
  from cpo_vor c cross join gbsimple g
);  -- the st_within check speeds this up hugely, but it still takes about 10m
create index clipped_idx on cpo_clipped using gist(geom);
analyze cpo_clipped;
' | psql -d codepointopen

echo "Calculating bounding boxes ..."

echo '
create table bboxes as (
  select 
    c.outward,
    st_setsrid(st_extent(cc.geom), 27700) as geom
  from cpo c
  join cpo_clipped cc on st_within(c.pt, cc.geom)
  group by c.outward
);

create view csvdata as (
  with boxcoords as (
    select 
      outward,
      cast(floor(st_xmin(geom)) as integer) as e1,
      cast(floor(st_ymin(geom)) as integer) as n1,
      cast(ceil(st_xmax(geom)) as integer) as e2,
      cast(ceil(st_ymax(geom)) as integer) as n2
    from bboxes
  )
  select
    outward,
    e1 as originE,
    n1 as originN,
    e2 - e1 as sizeE,
    n2 - n1 as sizeN
  from boxcoords
);
' | psql -d codepointopen

echo "Writing CSV data ..."

psql -d codepointopen -c '\copy (select * from csvdata) to stdout csv' > outwardbboxes.csv

echo "Dropping database ..."

dropdb codepointopen

echo "Done."
