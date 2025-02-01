#pragma once

#include "../core/Common.h"

struct IDxcBlob;

enum class ShaderType
{
	VERTEX_SHADER = 0,
	PIXEL_SHADER,
	COMPUTE_SHADER,
	LIB_SHADER, // For workgraphs and raytracing shaders
	NUMBER_SHADER_TYPES,
};

struct ShaderDesc
{
	ShaderType m_type;
	std::string m_file_name;
	std::string m_entry_point_name;
};

struct Shader
{
	ComPtr<IDxcBlob> m_blob;
	ShaderDesc m_shader_desc;
};