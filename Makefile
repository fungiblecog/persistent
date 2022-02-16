.PHONY: all
.PHONY: list
.PHONY: vector
.PHONY: hashmap
.PHONY: clean

PATH_LIST := ./list/
PATH_VECTOR := ./vector/
PATH_HASHMAP := ./hashmap/

all: list vector hashmap

list:
	make -C $(PATH_LIST)

vector:
	make -C $(PATH_VECTOR)

hashmap:
	make -C $(PATH_HASHMAP)

clean:
	make -C $(PATH_LIST) clean
	make -C $(PATH_VECTOR) clean
	make -C $(PATH_HASHMAP) clean
