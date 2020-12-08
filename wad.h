#pragma once
#include <iostream>
#include <string>
#include <vector>

class Wad {

    struct Descriptor{
        uint32_t elementOffset;
        uint32_t elementLength;
        std::string name;
        std::string directoryName;
        std::string fileName;
        std::vector<char> elementData;
    };
    
    struct Node{
        struct Descriptor key;
        std::vector<Node *> child;
    };
    
    Node *newNode(struct Descriptor key);
    
private:
    std::string fileData;
    
    std::string fh_magic;
    uint32_t fh_numDescriptors;
    uint32_t fh_descripOffset;
    
    std::vector<Descriptor> descriptorStore;
    
    Node* ft_root;
    
    
public:
    Wad(const std::string &path);
    void parseFileHeader();
    void parseDescriptors();
    std::vector<char> grabFileContents(struct Descriptor curDescriptor);
    Node* createFileTree();
    Node* searchFileTree_1Level(Node* root, std::string fileName);
    int searchFileTree_1Level(Node* root, std::vector<std::string> *directory);
    Node* searchFileTree_Path(const std::string &path);
    int searchFileTree_Path(std::vector<std::string> *directory);
    
    
    static Wad* loadWad(const std::string &path);
    char* getMagic();
    bool isContent(const std::string &path);
    bool isDirectory(const std::string &path);
    int getSize(const std::string &path);
    int getContents(const std::string &path, char *buffer, int length, int offset = 0);
    int getDirectory(const std::string &path, std::vector<std::string> *directory);
    
};
