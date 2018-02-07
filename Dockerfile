FROM anotherfoxguy/ror-build-box:master

RUN mkdir -p /src/
WORKDIR /src/

COPY . /src/
RUN cmake .
RUN make -j2
