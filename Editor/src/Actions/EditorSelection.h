#pragma once
#include "src/Core/Core.h"
#include "src/Scene/Entities/Entity.h"
#include "src/Resource/Resource.h"

enum EditorSelectionType {
	None,
	Entity, 
	File,
	Resource,
	Directory
};

class EditorSelection {
public:

	EditorSelectionType Type = EditorSelectionType::None;

	Nuake::Entity Entity;
	Ref<Nuake::File> File;
	Ref<Nuake::Directory> Directory;
	Nuake::Resource Resource;

	EditorSelection() 
	{
		Type = None;
	}

	EditorSelection(const Nuake::Entity& entity)
	{
		Type = EditorSelectionType::Entity;
		Entity = entity;
	}

	EditorSelection(const Ref<Nuake::File>& file)
	{
		Type = EditorSelectionType::File;
		File = file;
	}

	EditorSelection(const Ref<Nuake::Directory>& dir)
	{
		Type = EditorSelectionType::Directory;
		Directory = dir;
	}

	EditorSelection(const Nuake::Resource& resource)
	{
		Type = EditorSelectionType::Resource;
		Resource = resource;
	}

	operator bool()
	{
		return Type != None;
	}
};