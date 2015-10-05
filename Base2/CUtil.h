#ifndef CUTIL_H
#define CUTIL_H

#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include "dirent.h"
#include <math.h>
#include <iterator>
#include <list>
#include <string.h>

//-----------------Definciones-----------------------------------------------
//----------------------------------------------------------------------------

typedef unsigned int	uint;


//-----------------variables--------------------------------------------------
//----------------------------------------------------------------------------

#define _PI 3.14159265359
bool	cmpStrNum(const std::string &a, const std::string &b);


///////////////////////////FUNCIONES//////////////////////////////////////////
//funcion de comparacion por orden numerico
bool cmpStrNum(const std::string &a, const std::string &b)
{
	if (a.size() == b.size())return a < b;
	if (a.size() < b.size())return true;
	return false;
}
//---------------------------------------------------------------------------
//mostrar cutil_file_contenido de un puntero 
template <class t>
void ptr_mostrar(t a, int tam, std::ostream &os = std::cout)
{
	for (int i = 0; i < tam; i++)
	{
		os << a[i] << " ";
	}
	os << endl;
}
//---------------------------------------------------------------------------
//insert zeros into the front to get better renaming
//---------------------------------------------------------------------------
template <class t>
static t num_dig(t num)
{
	int res = 0;
	for (; num > 0; ++res)
		num /= 10;
	return res;
}
std::string insert_numbers(std::size_t innumber, std::size_t finaln, char token = '0')
{
	std::stringstream digits;
	for (auto i = num_dig(finaln) - num_dig(innumber); i > 0; --i)
		digits << token;
	digits << innumber;
	return digits.str();
}
/////////////////////////////////////////////////////////////////////////////
//-----------------grig generator--------------------------------------------
struct cuboid_dim{
	int xi, yi, xf, yf;
};
typedef cuboid_dim cutil_grig_point;
//w = frame width 
//h = frame height  
//cw = cuboid width
//ch = cuboid height
//ov_w = overlap_width
//ov_h = overlap_height
std::vector<cutil_grig_point> grid_generator(int w, int h, int cw, int ch, int ov_w, int ov_h)
{
	int wfin = w - cw + 1,
		hfin = h - ch + 1;
	std::vector<cutil_grig_point> res;
	for (int i = 0; i < wfin; i += ov_w)
	{
		for (int j = 0; j < hfin; j += ov_h)
		{
			cutil_grig_point cuboid{ i, j, (i + cw - 1), (j + ch - 1) };
			res.push_back(cuboid);
		}
	}
	return res;
}
/////////////////////////////////////////////////////////////////////////////
//-----------------Parte del dirent------------------------------------------
#ifdef DIRENT_H
//-----------------------------------------------------
//!Requiere la biblioteca dirent.h y solo para windows
//
//
typedef std::vector<std::string>	cutil_file_cont;// estructura donde se guarda el resultado STD!!
//
void list_files(cutil_file_cont & sal, const char * d)
{
	sal.clear();
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(d)) != NULL) {
		while ((ent = readdir(dir)) != NULL)
		{
			sal.push_back(ent->d_name);
		}
		closedir(dir);
	}
	else
		perror("");
}
//Lista todos los archivos token de una carpeta 
void list_files(cutil_file_cont & sal, const char *d, const char * token)
{
	sal.clear();
	DIR *dir;
	struct dirent *ent;
	std::string  path(d);
	path += "/";
	if ((dir = opendir(d)) != NULL) {
		while ((ent = readdir(dir)) != NULL)
		{
			std::string fil = ent->d_name;
			if (fil.find(token) + 1)
				sal.push_back(path + ent->d_name);
		}
		closedir(dir);
	}
	else
		perror("");
}
//Lista todos los archivos token de una carpeta y subcarpetas
void list_files_all(cutil_file_cont & sal, const char *d, const char * token)
{
	DIR		*dir;
	struct	dirent *ent;
	std::string  path(d);
	path += "/";
	if ((dir = opendir(d)) != NULL) {
		while ((ent = readdir(dir)) != NULL)
		{
			std::string fil = ent->d_name;
			if (fil.find(token) + 1)
			{
				sal.push_back(path + ent->d_name);
			}
			else
			{
				if ((int)fil.find(".")<0)
					list_files_all(sal, std::string(path + ent->d_name).c_str(), token);
			}
		}
		closedir(dir);
	}
	else
		perror("");
}
//Lista todos los archivos token de una carpeta y subcarpetas ademas lista subcarpetas
void list_files_all(cutil_file_cont & sala, cutil_file_cont & salc, const char *d, const char * token)
{
	DIR		*dir;
	struct	dirent *ent;
	std::string  path(d);
	path += "/";
	if ((dir = opendir(d)) != NULL) {
		while ((ent = readdir(dir)) != NULL)
		{
			std::string fil = ent->d_name;
			if (fil.find(token) + 1){
				sala.push_back(path + ent->d_name);
			}
			else{
				if ((int)fil.find(".") < 0){
					salc.push_back(path + ent->d_name);
					list_files_all(sala, salc, std::string(path + ent->d_name).c_str(), token);
				}
			}
		}
		closedir(dir);
	}
	else
		perror("");
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//______________________________________________________________________________
//Data estructure that contains the file tree of a directory____________________
struct	DirectoryNode;
typedef std::vector<DirectoryNode*>	SonsType;
//Filelistype may be a pointer to reduce the memory space but is not much.......
typedef std::vector<std::string>	FileListType;

//______________________________________________________________________________
//Directory node simulates directory in the SO..................................
struct DirectoryNode{
	std::string		_label;
	SonsType		_sons;
	FileListType	_listFile;
	DirectoryNode	*_father = nullptr;
	DirectoryNode(std::string label, DirectoryNode * father) :
		_label(label),
		_father(father){}
	DirectoryNode(std::string label) :
		_label(label){}
	void Destroy()
	{

	}
};
std::ostream & operator << (std::ostream & os, DirectoryNode node)
{
	os << node._label << "-" << node._label << " ";
	return os;
}
//______________________________________________________________________________
////////////////////////////////////////////////////////////////////////////////
//this version of list files returns in the intial node the files with determi-.
//mate token that existing into the root node directory, if some directory con-.
//tains some target file then the flag is turned to true........................
void list_files_all_tree(DirectoryNode * node, const char * token)
{
	DIR		*dir;
	struct	dirent *ent;
	std::string  path = node->_label + "/";
	if ((dir = opendir(node->_label.c_str())) != NULL) {
		while ((ent = readdir(dir)) != NULL){
			std::string fil = ent->d_name;
			if (fil.find(token) + 1)
				node->_listFile.push_back(path + ent->d_name);
			else{
				if ((int)fil.find(".") < 0){
					DirectoryNode * file = new DirectoryNode(path + ent->d_name, node);
					node->_sons.push_back(file);
					list_files_all_tree(*(node->_sons.end() - 1), token);
				}
			}
		}
		closedir(dir);
	}
	else
		perror("");//if someone wants to say something in erro cases XD
}

#endif 
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
static std::size_t lastToken(std::string &base, std::string token)
{
	std::size_t	posf = 0,
		posa = base.find(token);
	for (; posa != std::string::npos; posa = base.find(token, posa + 1))
		posf = posa;
	return posf;
}

std::string cutil_GetLast(std::string & base, std::string token)
{
	return base.substr(0, lastToken(base, token));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string cutil_invert(std::string & base)
{
	std::string res;
	res.resize(base.length());
	for (size_t i = 0; i < base.length(); ++i){
		res[i] = base[i] == '/' ? '\\' : base[i];
	}
	return res;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::string cutil_LastName(std::string &base)
{
	auto p1 = lastToken(base, "/"),
		p2 = lastToken(base, ".");
	return base.substr(p1 + 1, (p2 - p1) - 1);
}

std::string cutil_antecessor(std::string & path, short step)
{
	std::string res = path;
	for (short i = 0; i < step; ++i)
	{
		auto pos = lastToken(res, "/");
		res = res.substr(0, pos);
	}
	return cutil_LastName(res);
}

#endif 
