/*
 * main.cpp
 *
 *  Created on: Jan 4, 2026
 *      Author: vitya
 */

#include "stdio.h"

#include "jsontools.h"

TJsonNode jroot;

void test_write()
{
	printf("Testing JSON write\n");

	jroot.Clear();
	jroot.Add("NODE_STR1", "asdf");
	jroot.Add("NODE_STR2", "xyz");
	jroot.Add("NODE_NUM", 5.1);
	jroot.Add("NODE_BOOL", false);
	jroot.Add("NODE_INT", 5678983479879378439);

	TJsonNode & jnode = jroot.Add("SUBNODE", nkObject);
	jnode.Add("STR1", "qwerty");
	jnode.Add("NUM", 3.14);

	jroot.SaveToFile("test.json");

	printf("JSON text: %s\n", jroot.GetAsString().c_str());
}

void test_read()
{
	printf("Testing JSON read\n");

	jroot.Clear();
	printf("JSON after clear: %s\n", jroot.GetAsString().c_str());

	jroot.LoadFromFile("test.json");

	printf("JSON text: %s\n", jroot.GetAsString().c_str());

	TJsonNode * jv;
	if (jroot.Find("NODE_STR1", jv))
	{
		printf("NODE_STR1 = %s\n", jv->GetAsString().c_str());
	}

	if (jroot.Find("SUBNODE.STR1", jv))
	{
		printf("SUBNODE.STR1 = %s\n", jv->GetAsString().c_str());
	}
}

int main()
{
	printf("JSONTOOLS Test\n");

	test_write();

	test_read();

	//TJsonNode & jnode;
	//TJsonNode & jv;


	return 0;
}
