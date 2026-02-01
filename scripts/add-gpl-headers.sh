#!/bin/bash
# Script to add GPL-3.0 license headers to all source files
# Run from project root: ./scripts/add-gpl-headers.sh

GPL_HEADER="/*
 * BetterWallpaper - Modern animated wallpaper manager for Linux
 * Copyright (C) 2024-2026 Onxy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
"

# Find all .cpp and .hpp files in src/
find src/ -name "*.cpp" -o -name "*.hpp" | while read file; do
    # Check if file already has a license header
    if ! head -1 "$file" | grep -q "/* "; then
        echo "Adding header to: $file"
        # Create temp file with header + original content
        echo "$GPL_HEADER" > "$file.tmp"
        cat "$file" >> "$file.tmp"
        mv "$file.tmp" "$file"
    else
        echo "Skipping (already has header): $file"
    fi
done

echo "Done! Headers added to all source files."
