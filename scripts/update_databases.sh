#!/bin/bash

set -e
UPDATE_NEEDED=false

# Convert colon-separated list to array
IFS=':' read -ra FILE_ARRAY <<< "$GEOIP_FILES"

# Check the database files are more than 1 day old
for file in "${FILE_ARRAY[@]}"; do
    if [ -f "$GEOIP_DATABASE_DIR/$file" ]; then
        if [ $(stat -c %Y "$GEOIP_DATABASE_DIR/$file") -lt $(date -d "1 day ago" +%s) ]; then
            echo "Database file $file is more than 1 day old, updating..."
            UPDATE_NEEDED=true
            break
        fi
    else
        echo "Database file $file not found, will download..."
        UPDATE_NEEDED=true
        break
    fi
done

if [ "$UPDATE_NEEDED" = true ]; then
    echo "Updating GeoIP databases..."
    $GEOIP_UPDATE_TOOL_BIN -f $GEOIP_DATABASE_DIR/GeoIp.conf -d $GEOIP_DATABASE_DIR

    # Touch files to update modification time
    for file in "${FILE_ARRAY[@]}"; do
        touch "$GEOIP_DATABASE_DIR/$file"
    done

    # Call endpoint to reload the databases
    if [ ! -z "$GEOIP_RELOAD_ENDPOINT" ]; then
        echo "Reloading databases via $GEOIP_RELOAD_ENDPOINT ..."
        curl -X POST $GEOIP_RELOAD_ENDPOINT
    fi

    echo "Database update complete"
fi
