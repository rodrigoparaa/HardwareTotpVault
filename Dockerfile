FROM ghcr.io/pspdev/pspdev:latest

WORKDIR /src
COPY . /src

CMD ["sh", "-c", "make clean && make"]
