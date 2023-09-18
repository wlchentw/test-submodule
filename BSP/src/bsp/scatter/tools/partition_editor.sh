#!/bin/bash

if [ -z "$1" ]
then
	# Show the partition layout if no argument input
	echo 'Usage:'
	echo '	Show current Partitions layout:'
	echo '		sh partition_editor.sh ${XMLFILE}'
	exit 1
fi

if [ ! -f "$1" ]
then
	echo $1': File not exist!'
	exit 1
fi

# Show the partition layout in better interactive mode
tmpfile=".part.tmp"
xmlfile=$1

# LBS(size defined in tmp file is KB based)
lbs_exist=`grep -o "\<lbs\>" ${xmlfile}`
if [ -z $lbs_exist ]
then
	# For eMMC/Nor, only support 1024-aligned by now
	lbs=1
else
	lbs=`awk -F'"' '/lbs=/{print $4}' ${xmlfile}`
	lbs=$(($lbs/1024))
fi
# LBA(Total page number)
lba_exist=`grep -o "\<lba\>" ${xmlfile}`
if [ -z $lba_exist ]
then
        lba=0
else
        lba=`awk -F'"' '/lba=/{print $2}' ${xmlfile}`
fi
# reserved some blocks to store BBT
reserved_exist=`grep -o "\<reserved\>" ${xmlfile}`
if [ -z $reserved_exist ]
then
	reserved=0
else
	reserved=`awk -F'"' '/reserved=/{print $6}' ${xmlfile}`
fi

# Generate temporary file for interactive modifing and editing
echo '*** You can delete/insert/modify(partname/size) partitions ***' > ${tmpfile}
echo '*** Partition size must be ERASE_BLOCK_SIZE(define in Nand SPEC) aligned ***' >> ${tmpfile}
echo '' >> ${tmpfile}
echo 'Part_name			Size(KB)        Attr' >> ${tmpfile}
`sed -nr 's/^.+start=\"(\w+)\"\s*end=\"(\w+)\"\s*(attributes=\"(\w+)\"\s*)?name=\"(\w+)\".+$/\5 \1 \2 \4/p' ${xmlfile} |
	awk -v blk_size=$lbs '{printf "%-15s\t\t%d\t\t%s\n", $1, ($3-$2+1)*blk_size, $4}' >> ${tmpfile}`

# Edit the partition layout via vim
vim ${tmpfile}

# The new partition layout
part_name=(`awk '/^\w+\s+[0-9]+/{print $1}' ${tmpfile}`)
part_size=(`awk '/^\w+\s+[0-9]+/{print $2}' ${tmpfile}`)
part_attr=(`awk '/^\w+\s+[0-9]+/{if ($3 == "") print "-"; else print $3}' ${tmpfile}`)
# How many elements in the array
newcnt=$((${#part_name[@]}-1))
# Generate the temporary xml file
xmltmp=".xml.tmp"
`cp -f ${xmlfile} ${xmltmp}`
`sed -i '/name=/d' ${xmltmp}`
# Modify the temporary xml file
str1='	<entry type="{0FC63DAF-8483-4772-8E79-3D69D8477DE4}" start="'
str2='" end="'
strattr='" attributes="'
str3='" name="'
str4='" />'
# first start address
sa=`awk -F'"' '/name=/{print $4; exit;}' ${xmlfile}`
# last end address
lea=`sed -n '/name=/p' ${xmlfile} | sed -n '$p' | awk -F'"' '{print $6}'`
for ((i=0; i<=$newcnt; i++)) {
	len=${part_size[i]}
	if [ $i -eq $newcnt ]
	then
		ea=$lea
		if [ $lba -ne 0 -a $reserved -ne 0 ]
		then
			ea=$(($lba-$reserved-1))
		fi
	else
		ea=$(($sa+$len/$lbs-1))
	fi
	name=${part_name[i]}
	attr=${part_attr[i]}
	if [ "$attr" == "-" ]
	then
		`sed -i '/<\/partition>/i \'"$str1$sa$str2$ea$str3$name$str4"'' ${xmltmp}`
	else
		`sed -i '/<\/partition>/i \'"$str1$sa$str2$ea$strattr$attr$str3$name$str4"'' ${xmltmp}`
	fi
	sa=$(($ea+1))
}
# Replace the xml file with the temp xml file, delete useless temporary files
`cp -f ${xmltmp} ${xmlfile}`
`rm -f ${xmltmp} ${tmpfile}`

exit 0
