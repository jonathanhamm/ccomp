make

sources=$(find samples | grep '\.pas$')

for i in $sources; do
  ./pc $i
done

tac=$(find samples | grep '\.tac$')

rm samples/*.pretty 2>/dev/null

for i in $tac; do
  pr -w 180 -2 < $i > $i".pretty"
done





