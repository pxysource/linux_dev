###############################################################################
# @file auto_input_no_name.sh
# @author panxingyuan (panxingyuan1@163.com)
# @brief Auto input no and name in script. 
# @version 0.1
# @date 2023-12-23
#       Create this file.
# @copyright Copyright (c) 2023
###############################################################################

#!/bin/sh

# 方法一：重定向
./please_input_no_name.sh < ./input.data

# 方法二：管道
echo -e "1\nxxx\n" | ./please_input_no_name.sh
