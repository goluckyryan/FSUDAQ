#!/bin/bash

if [ $# -eq 0 ]; then
    echo "Usage: $0 file_pattern"
    echo "example:"
    echo "    $0  \"test_002*.fsu\""
    exit 1
fi

file_pattern="$1"

# Iterate through each file in the directory
for file in ${file_pattern}; do

  echo ${file}

  if [[ $file == *QDC* ]]; then
    # Insert "16" before the last underscore
    new_name="${file%_*}_16_${file##*_}"
  else
    # Insert "4" before the last underscore
    new_name="${file%_*}_4_${file##*_}"
  fi
  
  # Rename the file
  mv -iv "$file" "$new_name"
  
  echo "Renamed: $file to $new_name"

done
