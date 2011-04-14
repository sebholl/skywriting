for f in ./bin/$1/*
do
    ln -f -s ./$1/${f##*/} ./bin/${f##*/}
done
