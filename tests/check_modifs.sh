#/bin/bash

export LANG=C
export LC_ALL=C

mkdir -p mnt
./tagfs $PWD/images $PWD/mnt
sleep 1
mkdir mnt/ecole2
mkdir mnt/ecole
ln mnt/ipb.jpeg mnt/ecole2
mv mnt/ecole2/ipb.jpeg mnt/ecole/
ln mnt/marmotte.jpeg mnt/gentil/
rm mnt/mechant/rabbit.jpeg
rm mnt/monty/animal/rabbit.jpeg
mv mnt/mechant/coyote.jpeg mnt/gentil/cartoon/coyote.jpeg

fusermount -u mnt

(cat <<EOF
[bilbo.jpeg]
cartoon
gentil
hobbit

[coyote.jpeg]
animal
mechant

[gru.jpeg]
gentil

[ipb.jpeg]
ecole

[marmotte.jpeg]
animal
gentil

[rabbit.jpeg]
animal
monty

EOF
) > check_$$

diff $PWD/images/.tags.new check_$$ > diff_$$
if test -s diff_$$
then
    echo "Test ... Failure"
    cat diff_$$
else
    echo "Test ... Pass"
fi
rm -f check_$$ diff_$$

