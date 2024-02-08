count=0
sum=0

while IFS= read -r line
do
  sum=$(echo "$sum+$line" | bc)
  ((count++))
done < out.txt

echo "scale=3; $sum / $count" | bc