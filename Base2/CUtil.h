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
	if (a.size() < b.size() )return true;
	return false;
}
//---------------------------------------------------------------------------
//mostrar cutil_file_contenido de un puntero 
template <class t>
void ptr_mostrar(t a, int tam, std::ostream &os = std::cout)
{
	for (int i = 0; i < tam; i++)
	{
		os << a[i] <<" ";
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
	for (auto i = num_dig(finaln) - num_dig(innumber); i >= 0; --i)
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
	int wfin = w - cw,
		hfin = h - ch;
	std::vector<cutil_grig_point> res;
	for (int i = 0; i < wfin; i+= ov_w)
	{
		for (int j = 0; j < hfin; j += ov_h)
		{
			cutil_grig_point cuboid{i,j,i+cw,j+ch};
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
void list_files_all(cutil_file_cont & sala,cutil_file_cont & salc, const char *d, const char * token)
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
				sala.push_back(path + ent->d_name);
			}
			else
			{
				if ((int)fil.find(".") < 0)
				{
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
#endif 
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/*template <typename T>
struct is_pointer
{
	static const bool value = false;
};

template <typename T>
struct is_pointer<T*>
{
	static const bool value = true;
};/**/
//------------final----------------------
#endif 