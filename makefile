cc=g++
VIDEO=Videoer

$(VIDEO):Videoer.cc
	$(cc) $^ -o $@ -std=c++11 -ljsoncpp -L/usr/lib64/mysql -lmysqlclient -lpthread

clean:
	rm -f $(VIDEO)
