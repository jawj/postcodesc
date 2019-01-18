

    # generate C struct arrays from latest CodePoint Open CSV files
    ./postcodes.rb /path/to/codepo_gb/Data/CSV/

    # compile the testing tool
    gcc postcodes/*.c -std=gnu99 -D_GNU_SOURCE -Wall -Wno-missing-braces -O2 -o postcodesc

    # try it
    ./postcodesc sw1a0aa

