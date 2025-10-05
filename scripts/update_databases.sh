#!/bin/bash

set -e
UPDATE_NEEDED=false
# Check the database files are more than 1 day old
for file in $GEOIP_FILES; do
    if [ $(stat -c %Y $file) -lt $(date -d "1 day ago" +%s) ]; then
        echo "Database file $file is more than 1 day old, updating..."
        UPDATE_NEEDED=true
        break
    fi
done

if [ "$UPDATE_NEEDED" = true ]; then
    $GEOIP_UPDATE_TOOL_BIN -f $GEOIP_DATABASE_DIR/GeoIp.conf -d $GEOIP_DATABASE_DIR
    for file in $GEOIP_FILES; do
        touch $GEOIP_DATABASE_DIR/$file
    done

    # Call endpoint to reload the databases
    if [ ! -z "$GEOIP_RELOAD_ENDPOINT" ]; then    
        echo "Reloading databases via $GEOIP_RELOAD_ENDPOINT ..."
        curl -X POST $GEOIP_RELOAD_ENDPOINT
    fi
fi
