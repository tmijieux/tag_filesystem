#/bin/bash

export LANG=C
export LC_ALL=C

mkdir -p mnt
./tagfs $PWD/images $PWD/mnt
sleep 1
TOUS=$(/bin/ls mnt | tr ' ' '\012' | sort)
ANIMAL=$(/bin/ls mnt/animal | tr ' ' '\012' | sort | tr '\012' ' ')
ANIMAL_MECHANT=$(/bin/ls mnt/animal/mechant | tr ' ' '\012' | sort | tr '\012' ' ')
ANIMAL_MECHANT_MONTY=$(/bin/ls mnt/animal/mechant/monty | tr ' ' '\012' | sort | tr '\012' ' ')
ANIMAL_MECHANT_MONTY_V2=$(/bin/ls mnt/monty/animal/mechant/ | tr ' ' '\012' | sort | tr '\012' ' ')
INEXISTANT=$(/bin/ls mnt/foo 2>&1)
fusermount -u mnt

IMAGES=$(ls $PWD/images)
TAGS=$(grep -v "\[" $PWD/images/.tags | grep -v "^$"|grep -v "#"|sort|uniq)
TAGS_IMAGES=$(echo $TAGS $IMAGES | tr ' ' '\012'| sort)

echo "verification"
if test "$TOUS" == "$TAGS_IMAGES"
then
    echo "Test1 ... Pass"
else
    echo "Test1 ... Failure"
    echo "Expected:"
    echo $TAGS_IMAGES
    echo "Obtained:"
    echo $TOUS
fi

ANIMAL_EXPECTED="coyote.jpeg marmotte.jpeg mechant monty rabbit.jpeg "
if test "$ANIMAL" == "$ANIMAL_EXPECTED"
then
    echo "Test2 ... Pass"
else
    echo "Test2 ... Failure"
    echo "Expected:"
    echo "'$ANIMAL_EXPECTED'"
    echo "Obtained:"
    echo "'$ANIMAL'"
fi

ANIMAL_MECHANT_EXPECTED="coyote.jpeg monty rabbit.jpeg "
if test "$ANIMAL_MECHANT" == "$ANIMAL_MECHANT_EXPECTED"
then
    echo "Test3 ... Pass"
else
    echo "Test3 ... Failure"
    echo "Expected:"
    echo "'$ANIMAL_MECHANT_EXPECTED'"
    echo "Obtained:"
    echo "'$ANIMAL_MECHANT'"
fi

ANIMAL_MECHANT_MONTY_EXPECTED="rabbit.jpeg "
if test "$ANIMAL_MONTY_MECHANT" == "$ANIMAL_MONTY_MECHANT_EXPECTED"
then
    echo "Test4 ... Pass"
else
    echo "Test4 ... Failure"
    echo "Expected:"
    echo "'$ANIMAL_MONTY_MECHANT_EXPECTED'"
    echo "Obtained:"
    echo "'$ANIMAL_MONTY_MECHANT'"
fi

if test "$ANIMAL_MECHANT_MONTY" == "$ANIMAL_MECHANT_MONTY_V2"
then
    echo "Test5 ... Pass"
else
    echo "Test5 ... Failure"
    echo ""
fi

INEXISTANT_EXPECTED="/bin/ls: cannot access mnt/foo: No such file or directory"
if test "$INEXISTANT" == "$INEXISTANT_EXPECTED"
then
    echo "Test6 ... Pass"
else
    echo "Test6 ... Failure"
    echo "Expected:"
    echo "'$INEXISTANT_EXPECTED'"
    echo "Obtained:"
    echo "'$INEXISTANT'"
fi
