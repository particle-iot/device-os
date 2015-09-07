FROM particle/buildpack-hal

COPY . /firmware
COPY ./docker /

RUN /scripts/build-all-platforms.sh

WORKDIR /
