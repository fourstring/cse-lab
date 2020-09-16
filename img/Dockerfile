FROM ddnirvana/cselab_env:latest
USER root
RUN apt update && \
    apt install -y gdbserver
RUN sh -c "$(wget -O- https://github.com/deluan/zsh-in-docker/releases/download/v1.1.1/zsh-in-docker.sh)" -- \
     -t robbyrussell -p git

CMD /bin/zsh