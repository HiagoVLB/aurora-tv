FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates \
    && sed -i 's|http://|https://|g' /etc/apt/sources.list.d/ubuntu.sources \
    && apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates \
       build-essential cmake ninja-build pkg-config git gettext \
       libsdl2-dev libsdl2-image-dev libopus-dev libcurl4-openssl-dev uuid-dev \
       libavcodec-dev libavutil-dev libswscale-dev libexpat1-dev libmbedtls-dev \
       libfontconfig1-dev libfreetype6-dev libevdev-dev libudev-dev libcec-dev \
       xvfb xauth python3 python3-pil fonts-dejavu-core libgl1-mesa-dri libglx-mesa0 \
    && rm -rf /var/lib/apt/lists/*
ENV SDL_RENDER_DRIVER=opengl LIBGL_ALWAYS_SOFTWARE=1 SDL_AUDIODRIVER=dummy
WORKDIR /source
