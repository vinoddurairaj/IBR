#!/bin/sh

# make AIX exported symbols table

objlst=$*

# text symbols 

cat << EOHDR
#!/unix
*
*
************************************
* Exported ftd driver text symbols *
************************************

EOHDR

echo ${objlst} |\
while read objfil
do
	nm -Bx ${objfil} 2> /dev/null |\
	while read addr typ nam
	do
		if [ "${typ}" = "T" ]
		then
			echo ${nam} | sed -e '\@^\.@s@@@'
		fi
	done
done

# ad hoc misc syms, data for example...

cat << EOHDR

************************************
* Exported ftd driver data symbols *
************************************

ftd_global_state
chunk_size
max_bab_buffers_in_use
bab_mgrs
num_free_bab_buffers
free_bab_buffers
ftd_lg_state
ftd_buf_pool
ftd_buf_pool_count
ftd_buf_pool_size
trbblk_tab
  
EOHDR
