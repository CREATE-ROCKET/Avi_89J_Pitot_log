# コンパイラとターゲット設定
CC = g++
TARGET = main.out
SRCS = main.cpp
OBJS = $(SRCS:.cpp=.o)

# ビルドルール
$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CC) -c $< -o $@

# 全ビルドとクリーンアップ
all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

# 実行
run: $(TARGET)
	./$(TARGET)
