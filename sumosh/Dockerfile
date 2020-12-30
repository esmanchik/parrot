FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y g++ rsync zip openssh-server make cmake git curl

RUN apt-get install -y \
    git build-essential autoconf libtool \
    python python3 zlib1g-dev libavahi-client-dev \
     libavcodec-dev libavformat-dev libswscale-dev libncurses5-dev mplayer

RUN useradd parrot
RUN mkdir /home/parrot
RUN chown parrot:parrot /home/parrot

# RUN sudo -u parrot echo PATH="${HOME}/.bin:${PATH}" >> /home/parrot/.bashrc

USER parrot
WORKDIR /home/parrot
RUN mkdir -p .bin
RUN curl https://storage.googleapis.com/git-repo-downloads/repo > .bin/repo
RUN chmod a+rx .bin/repo
RUN git config --global user.name parrot && git config --global user.email parrot@example.com
# RUN locale -a
RUN export PATH="${HOME}/.bin:${PATH}" LANG="C.UTF-8" && \
    repo init -m release.xml -u https://github.com/Parrot-Developers/arsdk_manifests.git && \
    repo sync && ./build.sh -p arsdk-native -t build-sdk -j && \
    ./build.sh -p arsdk-native -t build-sample-JumpingSumoSample -j

RUN mkdir -p Desktop/robot/parrot/sdk
RUN cd Desktop/robot/parrot/sdk && ln -s ~/out && cd
RUN git clone https://github.com/esmanchik/parrot.git esmanchik
RUN mkdir esmanchik/sumosh/build
RUN . ~/out/arsdk-native/staging/native-wrapper.sh && cd esmanchik/sumosh/build && cmake .. && make
