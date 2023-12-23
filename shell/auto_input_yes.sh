###############################################################################
# @file auto_input_yes.sh
# @author panxingyuan (panxingyuan1@163.com)
# @brief Auto input yes in shell script. 
# @version 0.1
# @date 2023-12-23
#       Create this file.
# @copyright Copyright (c) 2023
###############################################################################

#!/bin/sh

echo aaaaaaaaa > a.txt
echo bbbbbbbbb > b.txt
echo "file content:"
echo "--------------------------"
echo "a.txt"
cat a.txt
echo "--------------------------"
echo "b.txt"
cat b.txt

echo -e "yes\n" | cp -i a.txt b.txt

echo ""
echo "copy, file content:"
echo "--------------------------"
echo "a.txt"
cat a.txt
echo "--------------------------"
echo "b.txt"
cat b.txt

rm a.txt
rm b.txt
