numfiles=$(ls -1p | grep -v --regexp='^.*[/]\{1\}' | wc -l)
for ((i=1; i <= numfiles; i++))
do
	name=$(ls -1p | grep -v --regexp='^.*[/]\{1\}' | head -n $i | tail -n 1)
	echo $name
	iconv -f utf8 -t cp1251 < $name > convertcp1251/$name.cp1251
done
numfiles=$(ls -1p | grep -v --regexp='^.*[/]\{1\}' | wc -l)
for ((i=1; i <= numfiles; i++))
do
	name=$(ls -1p | grep -v --regexp='^.*[/]\{1\}' | head -n $i | tail -n 1)
	echo $name
	iconv -f utf8 -t koi8r < $name > convertkoi8r/$name.koi8r
done
numfiles=$(ls -1p | grep -v --regexp='^.*[/]\{1\}' | wc -l)
for ((i=1; i <= numfiles; i++))
do
	name=$(ls -1p | grep -v --regexp='^.*[/]\{1\}' | head -n $i | tail -n 1)
	echo $name
	indent -gnu $name -o indent/$name
done