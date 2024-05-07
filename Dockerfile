#Warning! this file requires compiled C++ project, use CMake to make "RestfulMap" file!

FROM ubuntu:22.04

#ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y software-properties-common \
    && add-apt-repository -y ppa:pistache+team/unstable \
    && apt-get update \
    && apt-get install -y libpistache-dev \
    && rm -rf /var/lib/apt/lists/*

COPY ./RestfulMap /usr/src/RestfulMap
RUN chmod +x /usr/src/RestfulMap

# Append to add storage.json data
RUN echo '{"log_path": "/usr/src/log.txt", "storage_path": "/usr/src/storage.json"}' > /usr/src/config.json

WORKDIR /usr/src/

EXPOSE 9080
ENTRYPOINT ["/usr/src/RestfulMap"]

#obsolete below
#CMD ["sh", "-c", "./RestfulMap || if [ $? -ne 0 ]; then exit 1; fi"]

