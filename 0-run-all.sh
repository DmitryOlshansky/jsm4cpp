#!/bin/bash
runner(){
	./1-*.sh
	./2-*.sh
	./3-*.sh
	./4-*.sh
	./5-*.sh
	./6-*.sh
}

runner 2> error-log.log
