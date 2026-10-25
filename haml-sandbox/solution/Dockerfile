# $ docker pull alpine:3.21
# sha256:48b0309ca019d89d40f670aa1bc06e426dc0931948452e8491e3d65087abc07d
FROM alpine:3.21@sha256:48b0309ca019d89d40f670aa1bc06e426dc0931948452e8491e3d65087abc07d

RUN apk add --no-cache python3

WORKDIR /solve
COPY solve.py /solve/solve.py

ENTRYPOINT ["python3", "/solve/solve.py"]
