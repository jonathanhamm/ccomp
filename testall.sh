make

sources=$(find samples | grep '\.pas$')

for i in $sources; do
  ./pc $i
done

tac=$(find samples | grep '\.tac$')

for i in $tac; do
  pr -2 $i >> $i
done
