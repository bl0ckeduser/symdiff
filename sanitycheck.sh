make clean 2>&1 >/dev/null
rm -f matcher
echo "compiling..."
if ! make CFLAGS="-DFLOATEVAL" 2>/dev/null >/dev/null;
then
	echo "compile failed"
	exit 1
fi

IFS='
'
for line in `cat sanitychecks.txt | grep -v '^#'`
do
	echo CHECK $line
	if ! echo $line | (./matcher 2>/dev/null >/dev/null);
	then
		echo "sanity check failed: $line"
		echo "----------------------------"
		echo "backlog: "
		echo $line | ./matcher
		exit 1
	fi
done

echo "all tests happy :D"
