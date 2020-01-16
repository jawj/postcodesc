# postcodes.c

This is a C library (and simple command-line tool) that implements postcode lookups in Great Britain based on Ordnance Survey’s [CodePoint Open](https://www.ordnancesurvey.co.uk/business-and-government/products/code-point-open.html) data set.

It's intended to power new features in a future version of my [GridPoint GB](http://mackerron.com/gridpointgb/) iOS app.

It's also intended to be:

* *reasonably compact* — each 5 – 8 character postcode with its associated easting, northing and (simplified) quality flag is stored in just a smidgen over 6 bytes, so that the full data set of over 1.7m items occupies under 10MB in the compiled binary (and standard `gzip` takes less than 10% off this)

* *reasonably quick* — postcode -> location lookups put binary search left, right and center (as it were), while location -> postcode lookups use a simple bounding-box index on the outward part

* *reasonably solid* — tests are built in

CodePoint Open data are compiled into the binary, which therefore needs quarterly updates to remain up to date. Scripts are included to process the postcode data into C code.

## Usage

    # generate proper bounding boxes to support reverse lookups, location -> postcode
    # (this step is not required if you only need forward lookups, postcode -> location)
    ./gen-bboxes.sh /path/to/codepoint-open/folder /path/to/boundaryline/shapefiles
    
    # generate C struct arrays, using bounding boxes if available
    ./gen-structs.rb /path/to/codepoint-open/folder

    # compile the testing tool
    gcc postcodes/*.c -Wall -Wno-missing-braces -O2 -o postcodesc

    # try it
    ./postcodesc sw1a0aa
    ./postcodesc bn1
    ./postcodesc 530300 181600
    ./postcodesc test
    

## Licence

I’m releasing the code under the [MIT licence](http://www.opensource.org/licenses/mit-license.php).
