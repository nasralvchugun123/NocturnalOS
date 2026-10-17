#!/bin/bash

cd "$(dirname "$0")/../.." || exit 1

read -p "enter searching str: " search_str
read -p "enter replace str:   " replace_str

if [ -z "$search_str" ] || [ -z "$replace_str" ]; then
    echo "<err>: you must something write."
    exit 1
fi

echo "finding in root: $(pwd)"
echo "of:   $search_str"
echo "to:  $replace_str"
echo "----------------------------------------"

if [[ "$OSTYPE" == "darwin"* ]]; then
    SED_INPLACE=(-i '')
else
    SED_INPLACE=(-i)
fi

find . -type f \( -name "*.h" -o -name "*.cpp" -o -name "*.c" -o -name "*.cc" -o -name "*.hpp" \) -exec grep -l "$search_str" {} + | while read -r file; do
    sed "${SED_INPLACE[@]}" "s|$search_str|$replace_str|g" "$file"
    echo "updated file: $file"
done

echo "----------------------------------------"
echo "done"