#!/bin/bash


NOCRASHFILE=$1
CRASHFILE=$2
echo $NOCRASHFILE
echo $CRASHFILE

echo "removing existing output files..."
rm futex*

echo "generating futex count files..."
grep "FUTEX (" $NOCRASHFILE | sort -k 3 | uniq -c --check-chars=35 --skip-fields=1 > futex_count_nocrash.txt
grep "FUTEX (" $NOCRASHFILE | sort -k 3 | uniq --check-chars=35 --skip-fields=1 > futex_unique_nocrash.txt
grep "FUTEX (" $CRASHFILE | sort -k 3 | uniq -c --check-chars=35 --skip-fields=1 > futex_count_module_crash.txt
grep "FUTEX (" $CRASHFILE | sort -k 3 | uniq --check-chars=35 --skip-fields=1 > futex_unique_module_crash.txt

echo "generating difference files. Empty file means no difference..."
awk 'NR==FNR{c[substr($4, length($4)-4, 5)]++;next};c[substr($4, length($4)-4, 5)] == 0' futex_count_nocrash.txt futex_count_module_crash.txt > futex_extra_calls_module_crash.txt
awk 'NR==FNR{c[substr($4, length($4)-4, 5)]++;next};c[substr($4, length($4)-4, 5)] == 0' futex_count_module_crash.txt futex_count_nocrash.txt > futex_extra_calls_nocrash.txt
awk 'NR==FNR{c[substr($4, length($4)-4, 5)]++;next};c[substr($4, length($4)-4, 5)] > 0' futex_count_nocrash.txt futex_count_module_crash.txt > futex_similar_calls.txt

echo "Attaching code backtrace with futex calls..."
while read p; do


  sed -n "/${p:21:22}/,/TRACER/p" $NOCRASHFILE  >> futex_withcode_backtrace_nocrash.txt
done <futex_unique_nocrash.txt

while read p; do
  sed -n "/${p:21:22}/,/TRACER/p" $CRASHFILE  >> futex_withcode_backtrace_crash.txt
done <futex_unique_module_crash.txt

echo "Gathering methods list..."
grep -P -o "^\t[\w/._/*/(/)]*" futex_withcode_backtrace_nocrash.txt | sort -k 3 | uniq | cat -n > futex_unique_methods_nocrash.txt
grep -P -o "^\t[\w/._/*/(/)]*" futex_withcode_backtrace_crash.txt | sort -k 3 | uniq | cat -n > futex_unique_methods_crash.txt
cat futex_withcode_backtrace_nocrash.txt futex_withcode_backtrace_crash.txt | grep -P -o "^\t[\w/._/*/(/)]*" | sort -k 3 | uniq | cat -n > futex_unique_methods_combined.txt

echo "Done"
