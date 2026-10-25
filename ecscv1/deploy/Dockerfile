FROM debian:trixie@sha256:9cc080028c43b27d2074d63a5f9caf7166d731494965616c1a6d2827a004585c

ENV FLAG="ECSC{not a real flag, for testing}"
RUN useradd --create-home --comment "CTF image user" ctf
WORKDIR /home/ctf
# libraries are default from this image, only provided to make local exploitation easier
COPY ld-linux-x86-64.so.2 libc.so.6 main ynetd .

EXPOSE 1024
CMD ["sh", "-c", "echo \"$FLAG\" > /home/ctf/flag && ./ynetd -u ctf /home/ctf/main"]
