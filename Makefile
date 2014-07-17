CXX=g++
CXXFLAGS=-std=c++11 -O3 -fopenmp -lz -I. -DKENLM_MAX_ORDER=6 -lrt
objs=lm/*.o util/*.o util/double-conversion/*.o
translator: main.o segmenter.o maxent.o myutils.o $(objs)
	$(CXX) -o cws main.o segmenter.o maxent.o myutils.o $(objs) $(CXXFLAGS)

main.o:main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp
segmenter.o:segmenter.cpp
	$(CXX) $(CXXFLAGS) -c segmenter.cpp
maxent.o:maxent.cpp
	$(CXX) $(CXXFLAGS) -c maxent.cpp
myutils.o:myutils.cpp
	$(CXX) $(CXXFLAGS) -c myutils.cpp

clean:
	rm *.o
