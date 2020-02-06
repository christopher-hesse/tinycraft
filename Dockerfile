FROM christopherhesse/dockertest:v5

RUN apt-get update
RUN apt-get install --yes --no-install-recommends xorg-dev libgl1-mesa-dev xvfb libosmesa6-dev
ADD env.yaml .
RUN conda env update --name env --file env.yaml