FROM ubuntu:16.04

RUN mkdir -p /src/
WORKDIR /src/

RUN apt-get update
RUN apt-get install -y curl wget
RUN curl -s https://packagecloud.io/install/repositories/AnotherFoxGuy/rigs-of-rods/script.deb.sh | bash
RUN apt-get install -y cmake pkg-config socketw-dev angelscript-dev libogre-1.9-dev libmygui-dev libmygui.ogreplatform0debian1v5 libopenal-dev libcurl4-openssl-dev libgtk2.0-dev libois-dev libssl-dev libwxgtk3.0-dev > apt-get-install.log
RUN wget -q http://ftp.debian.org/debian/pool/main/r/rapidjson/rapidjson-dev_1.1.0+dfsg-3_all.deb -Orapidjson-dev.deb
RUN dpkg -i rapidjson-dev.deb

COPY . /src/
RUN cmake .
RUN make -j2

RUN cat apt-get-install.log
