#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <queue>
#include <fcntl.h>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <unistd.h>
#include "wad.h"

Wad::Node* Wad::newNode(struct Descriptor key) {
    Node *temp = new Node;
    temp->key = key;
    return temp;
}

Wad::Wad(const std::string &path){
    this->fileData = path;
    
    parseFileHeader();
    parseDescriptors();
    this->ft_root = createFileTree();
}

void Wad::parseFileHeader() {
    std::ifstream file (this->fileData.c_str(), std::ios::in|std::ios::binary);
    
    if(!file.is_open()){
        std::cout << "open error" << std::endl;
    }
    std::string temp_magicValue;
    for(int i = 0; i < 4; i++){
        char temp;
        file.get(temp);
        temp_magicValue += temp;
    }
    
    uint32_t temp_numDescriptor = 0;
    for(int i = 0; i < 4; i++){
        char temp;
        file.get(temp);
        temp_numDescriptor += static_cast<unsigned char>(temp) << (8 * i);
        
    }
    
    uint32_t temp_descripOffset = 0;
    for(int i = 0; i < 4; i++){
        char temp;
        file.get(temp);
        temp_descripOffset += static_cast<unsigned char>(temp) << (8 * i);
        

    }
    
    file.close();
    
    this->fh_magic = temp_magicValue;
    this->fh_numDescriptors = temp_numDescriptor;
    this->fh_descripOffset = temp_descripOffset;
    
}

void Wad::parseDescriptors(){
    std::ifstream file (this->fileData.c_str(), std::ios::in|std::ios::binary);
    
    file.seekg (this->fh_descripOffset, std::ios::beg);
    
    
    for(int i = 0; i < this->fh_numDescriptors; i++){

        uint32_t temp_elementOffset = 0;
        for(int i = 0; i < 4; i++){
            char temp;
            file.get(temp);
            temp_elementOffset += static_cast<unsigned char>(temp) << (8 * i);
        }
        
        
        uint32_t temp_elementLength = 0;
        for(int i = 0; i < 4; i++){
            char temp;
            file.get(temp);
            temp_elementLength += static_cast<unsigned char>(temp) << (8 * i);
        }
        
        
        std::string temp_name;
        for(int i = 0; i < 8; i++){
            char temp;
            file.get(temp);
            temp_name += temp;
        }
        Wad::Descriptor curElement = {temp_elementOffset, temp_elementLength, temp_name};
//        std::cout << "This curElement offset: " << curElement.elementOffset << " length: " << curElement.elementLength << " name: " << curElement.name << std::endl;
        this->descriptorStore.push_back(curElement);

//        std::cout << "Current i value before increment. i: " << i << std::endl;
    }
    
    file.close();
    
}

std::vector<char> Wad::grabFileContents(struct Descriptor curDescriptor) {
    std::ifstream file (this->fileData.c_str(), std::ios::in|std::ios::binary);
    
    file.seekg (curDescriptor.elementOffset, std::ios::beg);
    
    
    std::vector<char> fileContent;
    for(int i = 0; i < curDescriptor.elementLength; i++){
        char temp;
        file.get(temp);
        fileContent.push_back(temp);
    }
    
    file.close();
    
    
    return fileContent;
    
}

Wad::Node* Wad::createFileTree(){
    std::stack<std::string> directoryNamespaceMarker;
    std::stack<struct Node*> directoryTreeNode;
    
    
    Wad::Descriptor rootDescriptor = {0, 0, "root"};
    Node* root = newNode(rootDescriptor);
    directoryTreeNode.push(root);
    
    
    bool flag_storedMapMarker = false;
    int count_MapMarker = 0;
    for(int i = 0; i < static_cast<int>(this->descriptorStore.size()); i++){
        bool isDirectory = this->descriptorStore.at(i).elementLength == 0;
        
        if(count_MapMarker == 10 && flag_storedMapMarker){
            flag_storedMapMarker = false;
            count_MapMarker = 0;
            //next file should not go in the directory of the MapMarker
            directoryNamespaceMarker.pop();
            directoryTreeNode.pop();
        }
        
        if(isDirectory && this->descriptorStore.at(i).name.find("_END") != std::string::npos){
            directoryNamespaceMarker.pop();
            directoryTreeNode.pop();
            //need to move current pointer to parent
            
            continue;
            
        }

        
        if(isDirectory){
                if (this->descriptorStore.at(i).name[0] == 'E' && isdigit(this->descriptorStore.at(i).name[1]) &&
                    this->descriptorStore.at(i).name[2] == 'M' && isdigit(this->descriptorStore.at(i).name[3]) ) {
                    this->descriptorStore.at(i).directoryName = this->descriptorStore.at(i).name.substr(0, 4);
                    
                    directoryNamespaceMarker.push(descriptorStore.at(i).name);
                    flag_storedMapMarker = true;
                    
                    //need to create as parent node
                    Node* directoryToPush = newNode( this->descriptorStore.at(i) );
                    directoryTreeNode.top()->child.push_back( directoryToPush );
                    directoryTreeNode.push( directoryToPush );
                    
                    continue;
                }
        }
        
        if(isDirectory){
            if(this->descriptorStore.at(i).name.find("_START") != std::string::npos){
                this->descriptorStore.at(i).directoryName = this->descriptorStore.at(i).name.substr(0, this->descriptorStore.at(i).name.find("_START"));
                
                directoryNamespaceMarker.push(this->descriptorStore.at(i).name.substr(0, this->descriptorStore.at(i).name.find("_START")));
                
                //need to create as parent node
                Node* directoryToPush = newNode( this->descriptorStore.at(i) );
                directoryTreeNode.top()->child.push_back( directoryToPush );
                directoryTreeNode.push( directoryToPush );
                
                continue;
                
            }
        }
        
        if(flag_storedMapMarker && count_MapMarker != 10){
            //isAFile
            this->descriptorStore.at(i).elementData = grabFileContents(this->descriptorStore.at(i));    //need to confirm this works | assigning contents of one vector to another
            
            std::string temp_fileName(this->descriptorStore.at(i).name);
            temp_fileName.erase(std::remove(temp_fileName.begin(), temp_fileName.end(), '\0'), temp_fileName.end());
            this->descriptorStore.at(i).fileName = temp_fileName;
            
            //need to add file as child of parent node (directory)
            Node* fileToPush = newNode( this->descriptorStore.at(i) );
            directoryTreeNode.top()->child.push_back( fileToPush );
            
            
            count_MapMarker++;
            continue;
        }
        
        this->descriptorStore.at(i).elementData = grabFileContents(this->descriptorStore.at(i));    //need to confirm this works
        
        std::string temp_fileName(this->descriptorStore.at(i).name);
        temp_fileName.erase(std::remove(temp_fileName.begin(), temp_fileName.end(), '\0'), temp_fileName.end());
        this->descriptorStore.at(i).fileName = temp_fileName;
        
        //need to add file as child of parent node (directory)
        Node* fileToPush = newNode( this->descriptorStore.at(i) );
        directoryTreeNode.top()->child.push_back( fileToPush );
        
    }
    
    return root;
}

Wad::Node* Wad::searchFileTree_1Level(Node* root, std::string fileName) {
    
    if (root==NULL)
        return nullptr;
    
    for(int i = 0; i < root->child.size(); i++){
        if(root->child[i]->key.name.compare(fileName) == 0 || root->child[i]->key.directoryName.compare(fileName) == 0 || root->child[i]->key.fileName.compare(fileName) == 0){
            return root->child[i];
        }
    }
    
    return nullptr;
}

int Wad::searchFileTree_1Level(Node* root, std::vector<std::string> *directory){
    if (root==NULL)
        return - 1;
    
    int count = 0;
    for(int i = 0; i < root->child.size(); i++, count++){
        if(root->child[i]->key.elementLength == 0){
            directory->push_back(root->child[i]->key.directoryName);
            continue;
        }
        
        directory->push_back(root->child[i]->key.fileName);
    }
    
    return count;
}

Wad::Node* Wad::searchFileTree_Path(const std::string &path) {
    Node* root_newLevel = nullptr;
    
        char str[path.length() + 1];
        strcpy(str, path.c_str());
        
        char * pch = strtok(str, "/");
        std::string pathName_toPass(pch);
        bool isFirst = true;
        
        while(pch != NULL){
            if(isFirst){
                root_newLevel = searchFileTree_1Level(this->ft_root, pathName_toPass);
                isFirst = false;
            }else{
                root_newLevel = searchFileTree_1Level(root_newLevel, pathName_toPass);
            }
            
            if(root_newLevel == nullptr){
                return nullptr;
            }
            
            pch = strtok(NULL, "/");
            pathName_toPass = "";
            if(pch != NULL){
                pathName_toPass = pch;
            }
        }
    
    return root_newLevel;
}

int Wad::searchFileTree_Path(std::vector<std::string> *directory){
    int countStore = searchFileTree_1Level(this->ft_root, directory);
    
    return countStore;
}

Wad* Wad::loadWad(const std::string &path) {
    Wad *currentWadObj = new Wad(path);
    
    return currentWadObj;
}

char* Wad::getMagic() {
    
    char* returnArray = new char[this->fh_magic.length() + 1];
    std::strcpy(returnArray, this->fh_magic.c_str());
    
    
    return returnArray;
}

bool Wad::isContent(const std::string &path) {
    if(path.compare("/") == 0 || path.empty()){
        return false;
    }
    
    Node* root_newLevel = searchFileTree_Path(path);
    
    if(root_newLevel == nullptr){
        return false;
    }else{
        return (root_newLevel->key.elementLength > 0);
    }
    
}

bool Wad::isDirectory(const std::string &path) {
    if(path.empty()){
        return false;
    }
    
    if(path.compare("/") == 0){
        return true;
    }
    
    Node* root_newLevel = searchFileTree_Path(path);
    
    if(root_newLevel == nullptr){
        return false;
    }else{
        return (root_newLevel->key.elementLength == 0);
    }
}

int Wad::getSize(const std::string &path) {
    if(path.empty()){
        return -1;
    }
    
    if(!isContent(path)){
        return -1;
    }
    
    Node* root_newLevel = searchFileTree_Path(path);
    
    return static_cast<int>( root_newLevel->key.elementData.size() );
}

int Wad::getContents(const std::string &path, char *buffer, int length, int offset) {
    if(path.empty()){
        return -1;
    }
    
    if(isDirectory(path)){
        return  -1;
    }
    
    if(!isContent(path)){
        return -1;
    }
    
    if(offset < 0){
        return -1;
    }
    
    Node* root_newLevel = searchFileTree_Path(path);
    
    if(root_newLevel == nullptr){
        return -1;
    }
    
    if((length + offset) > root_newLevel->key.elementData.size()){
        return -1;
    }
    
    if(offset > 0){
        int count = 0;
        for(int i = 0; i < length; i++, count++){
            buffer[i] = root_newLevel->key.elementData[i + offset];
        }
        
        return count;
    }
    
//    int index = 0;
    int count = 0;
    for(int i = 0; i < length; i++, count++){
        buffer[i] = root_newLevel->key.elementData[i];
    }
    
    return count;
    
}

int Wad::getDirectory(const std::string &path, std::vector<std::string> *directory) {
    if(isContent(path)){
        return -1;
    }
    
    if(path.empty()){
        return -1;
    }
    
    if(path.compare("/") == 0){
        return searchFileTree_Path(directory);
    }
    
    Node* root_newLevel = searchFileTree_Path(path);
    
    if(root_newLevel == nullptr){
        return -1;
    }
    
    int count = 0;
    for(int i = 0; i < static_cast<int>( root_newLevel->child.size() ); i++, count++){
        if(root_newLevel->child[i]->key.elementLength == 0){
            directory->push_back(root_newLevel->child[i]->key.directoryName);
            continue;
        }
        
        directory->push_back(root_newLevel->child[i]->key.fileName);
    }
    
    return count;
}
