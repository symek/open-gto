//******************************************************************************
// Copyright (c) 2002 Tweak Inc. 
// All rights reserved.
//******************************************************************************
#include <Gto/RawData.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <fnmatch.h>

using namespace Gto;
using namespace std;

bool verbose = false;

struct FullProperty
{
    string     name;
    Object*    object;
    Component* component;
    Property*  property;
};

typedef vector<FullProperty> FullProperties;

void gather(RawDataBase* db, FullProperties& all)
{
    for (int i=0; i < db->objects.size(); i++)
    {
        Object* o = db->objects[i];

        for (int j=0; j < o->components.size(); j++)
        {
            Component* c = o->components[j];

            for (int q=0; q < c->properties.size(); q++)
            {
                Property* p = c->properties[q];
        
                FullProperty fp;
                fp.name = o->name;
                fp.name += ".";
                fp.name += c->name;
                fp.name += ".";
                fp.name += p->name;
                fp.object = o;
                fp.component = c;
                fp.property = p;
        
                all.push_back(fp);
            }
        }
    }
}

void filter(RawDataBase* db,
            FullProperties& properties, 
            const char *include, 
            const char *exclude)
{
    for (int i=0; i < properties.size(); i++)
    {
        const FullProperty &fp = properties[i];
        bool deleteIt = false;

        if (include)
        {
            deleteIt = !fnmatch(include, fp.name.c_str(), 0) ? false : true;

            if (verbose && !deleteIt)
            {
                cout << "gtomerge: include pattern matched "
                     << fp.name << endl;
            }
        }

        if (exclude)
        {
            deleteIt = !fnmatch(exclude, fp.name.c_str(), 0) ? true : false;

            if (verbose && deleteIt)
            {
                cout << "gtomerge: exclude pattern matched "
                     << fp.name << endl;
            }
        }

        if (deleteIt)
        {
            Object *o = fp.object;
            Component *c = fp.component;
            Property *p = fp.property;

            c->properties.erase( find(c->properties.begin(), 
                                      c->properties.end(), 
                                      p) );

            if (c->properties.empty())
            {
                o->components.erase( find(o->components.begin(),
                                          o->components.end(),
                                          c) );

                if (o->components.empty())
                {
                    db->objects.erase( find(db->objects.begin(),
                                            db->objects.end(),
                                            o) );
                }
            }
        }
    }
}

void usage()
{
    cout << "USAGE: "
         << "gtofilter [options] -o outfile.gto infile.gto\n"
         << "-ie/--include regex    inclusion regex\n"
         << "-ee/--exclude regex    exclusion regex\n"
         << "-v                     verbose\n"
         << endl;
    
    exit(-1);
}
            
int main(int argc, char *argv[])
{
    char *inFile = 0;
    char *outFile = 0;
    char *includeExpr = 0;
    char *excludeExpr = 0;

    for (int i=1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-o"))
        {
            i++;
            if (i >= argc) usage();
            outFile = argv[i];
        }
        else if (!strcmp(argv[i], "-ie") || !strcmp(argv[i], "--include"))
        {
            i++;
            if (i >= argc) usage();
            includeExpr = argv[i];
        }
        else if (!strcmp(argv[i], "-ee") || !strcmp(argv[i], "--exclude"))
        {
            i++;
            if (i >= argc) usage();
            excludeExpr = argv[i];
        }
        else if (!strcmp(argv[i], "-v"))
        {
            verbose = true;
        }
        else if (*argv[i] == '-')
        {
            usage();
        }
        else
        {
            if (inFile) usage();
            inFile = argv[i];
        }
    }

    if (!inFile || !outFile)
    {
	cout << "no infile or outfile specified.\n" << flush;
        cout << endl;
        usage();
    }

    RawDataBaseReader reader;
    cout << "Reading input file " << inFile << "..." << endl;

    if (!reader.open(inFile))
    {
        cerr << "ERROR: unable to read file " << inFile
             << endl;
        exit(-1);
    }

    RawDataBase *db = reader.dataBase();
    FullProperties allProperties;
    gather(db, allProperties);
    filter(db, allProperties, includeExpr, excludeExpr);

    if (db->objects.empty())
    {
        cerr << "ERROR: everything was excluded" << endl;
        exit(-1);
    }

    RawDataBaseWriter writer;

    if (!writer.write(outFile, *db))
    {
	cerr << "ERROR: unable to write file " << outFile
	     << endl;
	exit(-1);
    }
    else
    {
	cout << "Wrote file " << outFile << endl;
    }

    return 0;
}