#!/bin/bash

# Loop through all files in the current directory
for file in *@2x.*; do
  # Check if the file exists to avoid errors
  if [[ -f "$file" ]]; then
    # Remove '@2x' from the filename
    new_name="${file//@2x/}"
    mv "$file" "$new_name"
    echo "Renamed: $file -> $new_name"
  fi
done

echo "All files renamed successfully."
