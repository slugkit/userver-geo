#!/bin/bash

# Export variables so that they are accessible by docker compose
export GEOIP_FILES=@GEOIP_FILES@
export GEOIP_DATABASE_DIR=./@GEOIP_DATABASE_DIR@
export GEOIP_UPDATE_TOOL_BIN=./@GEOIP_UPDATE_TOOL_BIN@
