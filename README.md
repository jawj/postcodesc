
This is a C library (and simple command-line tool) that implements UK postcode lookups based on Ordnance Survey’s CodePoint Open data set.

It's intended to power new features in a future version of my GridPoint GB iOS app.

It's also intended to be:

* reasonably compact — by making heavy use of bit fields and eliminating some further redundancy, it represents each 5 – 8 character postcode with its associated easting, northing and (simplified) quality flag in just a smidgen over 6 bytes, so that the full data set of over 1.7m items occupies just under 10MB in the compiled binary

* reasonably quick — binary searches are left, right and center)

* reasonably solid — it has some built-in tests

## Usage

    # generate C struct arrays from latest CodePoint Open CSV files
    ./postcodes.rb /path/to/codepo_gb/Data/CSV/

    # compile the testing tool
    gcc postcodes/*.c -std=gnu99 -D_GNU_SOURCE -Wall -Wno-missing-braces -O2 -o postcodesc

    # try it
    ./postcodesc sw1a0aa

## Licence

I’m releasing the code under the [MIT licence](http://www.opensource.org/licenses/mit-license.php).
