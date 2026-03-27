.PHONY: all push

IMAGE := vp-float-compiler:1

all:
	DOCKER_BUILDKIT=1 docker build -t $(IMAGE) .

push:
	docker push $(IMAGE)
