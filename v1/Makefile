PROJ := http-download
SOURCE :=  ${wildcard *.cc}
OBJS := ${patsubst %.cc, %.o, $(SOURCE)}
CURL-PKT := $(shell ls curl*.tar.gz)
CURL-DIR-TMP := $(patsubst  %.tar.gz, %,$(CURL-PKT))
CURL-DIR := curl
INC_DIR := -I$(CURDIR)/$(CURL-DIR)/include/curl
LIB_DIR := -L$(CURDIR)/$(CURL-DIR)/lib/.libs
LIBS := -lcurl -lpthread
CXXFLAGS += -Wall -DDEBUG
CXX := @g++
RM := @rm -fR

.PHONY: all clean curl clean-curl clean-all

$(PROJ):
	@echo "-------- [$@] compile starting ...... --------"
	$(CXX) -c $(SOURCE) $(INC_DIR) $(CXXFLAGS)
	$(CXX) -o $(PROJ) $(OBJS) $(LIB_DIR) $(LIBS)

all: curl $(PROJ)

curl:
	@echo "-------- [$@] compile starting ...... --------"
	$(RM) $(CURL-DIR)
	tar -xzf $(CURL-PKT)
	mv $(CURL-DIR-TMP) $(CURL-DIR)
	cd $(CURL-DIR); ./configure
	$(MAKE) -C $(CURL-DIR)

clean:
	@echo "######## Clean $(PROJ) starting ...... ########"
	$(RM) $(OBJS) $(PROJ)

clean-curl:
	@echo "######## Clean $(CURL-DIR) starting ...... ########"
	$(RM) $(CURL-DIR)
clean-all: clean clean-curl
