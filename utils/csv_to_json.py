#!/usr/bin/python
import csv  
import json  
import sys
  
# Create stdin reader
reader = csv.DictReader( sys.stdin, fieldnames = ( "timestamp","duration","operation","tag" ))  

# Parse the csv into json  
out = json.dumps( [ row for row in reader ], indent=4 )  

# Output the resulting json
print out
