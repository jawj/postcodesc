
This is a C library (and simple command-line tool) that implements UK postcode lookups based on Ordnance Survey’s CodePoint Open data set.

It's intended to power new features in a future version of my GridPoint GB iOS app.

It's also intended to be:

* *reasonably compact* — each 5 – 8 character postcode with its associated easting, northing and (simplified) quality flag is stored in just a smidgen over 6 bytes, so that the full data set of over 1.7m items occupies under 10MB in the compiled binary (and standard `gzip` takes less than 10% off this)

* *reasonably quick* — postcode -> location lookups make use of binary search left, right and center (as it were), while location -> postcode lookups use a simple bounding-box index on the ourward part

* *reasonably solid* — tests are built in

CodePoint Open data are compiled into the binary, which therefore needs quarterly updates to remain up to date. Scripts are included to process the postcode data into C code.

## Usage

    # generate proper bounding boxes to support reverse lookups, location -> postcode
    # (this step is not required if you only need forward lookups, postcode -> location)
    ./gen-bboxes.sh /path/to/codepoint-open/CSVs /path/to/boundaryline/shapefiles
    
    # generate C struct arrays, using bounding boxes if available
    ./gen-structs.rb /path/to/codepoint-open/CSVs

    # compile the testing tool
    gcc postcodes/*.c -std=gnu99 -D_GNU_SOURCE -Wall -Wno-missing-braces -O2 -o postcodesc

    # try it
    ./postcodesc sw1a0aa
    ./postcodesc 530300 181600
    ./postcodesc test
    

## Licence

I’m releasing the code under the [MIT licence](http://www.opensource.org/licenses/mit-license.php).
