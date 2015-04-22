== Example for building SeisComP3 in a Docker container ==
```
docker build -t seiscomp3-build .
cd ..
docker run --rm -v $PWD:/opt/src/seiscomp3 -it seiscomp3-build
```
