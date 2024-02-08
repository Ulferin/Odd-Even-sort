#!/bin/bash
args="${@:2}"
type="$1"
seed="$2"
m="$3"
max="$4"
nw="$5"
cache="$6"

aux_test="oe-sort${type}.txt"
ex="./oe-sort${type}"
res_file="${type}-res.txt"

echo "Testing ${type} time on ${args}" > $aux_test
for i in {1..5}
do
  case ${type} in
    "seq") eval "${ex} ${seed} ${m} ${max}" >> $aux_test ;;
    *) eval "${ex} ${seed} ${m} ${nw} ${cache} ${max}" >> $aux_test ;;
  esac
  
done

res=$(cat $aux_test | grep "Simulation spent:" | awk '{print $3}' > out.txt ; ./stats.sh)

echo "[${args}] -- avg: ${res}" >> $res_file
rm -f out.txt
rm -f $aux_test