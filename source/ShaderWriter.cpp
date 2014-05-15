#include "stdafx.h"
#include "ShaderWriter.h"

#include <sstream>

string shaderDir() {
	return sdkDir() + "/shaders";


}

ShaderWriter::ShaderWriter(const string& outFile) {

	m_Debug = true;

	m_Shader.open(outFile.c_str());
	
	m_Shader.precision(3);
	m_Shader.setf( std::ios::fixed, std:: ios::floatfield );
}

void ShaderWriter::includeFile(string filename, string fromFile) {

	if(fileExists(currentDir() + "/"+ filename)){
		filename = currentDir() + "/"+ filename;
	} else if (fileExists(dirName(fromFile) + "/"+ filename)){
		filename = dirName(fromFile) + "/"+ filename;
	} else if (fileExists(shaderDir() + "/"+ filename)){
		filename = shaderDir() + "/"+ filename;
	} else if(!fileExists(filename)) {
		cerr << "Could not find file: " << filename << endl;
		return;
	}

	ifstream file;
	file.open(filename);

	bool inComment = false;

	char _buff[200];
	while(!file.eof()) {
		file.getline(_buff,sizeof(_buff));

		string buff(_buff);
        
		if(!m_Debug) {
			if(buff[buff.size()-1] == '\r') {
				buff[buff.size()-1] =0;
			}

			if(inComment) {
				size_t i = buff.find("*/");
				if(i != -1){
					buff = buff.substr(i+2);
					inComment = false;
				} else {
					buff="";
				}

			}else{ 

				size_t i = buff.find("//");
				if(i != -1) {
					buff = buff.substr(0,i);
				}

				i = buff.find("/*");
				if(i != -1) {
					buff = buff.substr(0,i);
					inComment =true;
				}

				buff = stringReplace(buff,"\t","");
			}
		}

		if(buff.substr(0,8) == "#include"){
			includeFile(string(buff).substr(9),filename);
		}else{
			m_Shader << buff << endl;
		}
	}
}


inline string vec3string(SUVector3D vec) {
    stringstream ss;
    ss << "vec3(" << vec.x << "," << vec.y << "," << vec.z << ")";
    return ss.str();
}

inline string vec3string(SUPoint3D vec) {
    stringstream ss;
    ss << "vec3(" << vec.x << "," << vec.y << "," << vec.z << ")";
    return ss.str();
}

void ShaderWriter::writeLights(const vector<InstanceInfo>& instances) {

	m_Shader << "void applySceneLights() {";

	for(size_t i = 0; i < instances.size(); i ++) {

		InstanceInfo& light = *const_cast<InstanceInfo*>(&instances[i]);
        
        if(light.type != "light" && light.type != "spotlight" && light.type != "ambient")
            continue;
        
		SUVector3D colour = { 1, 1, 1 };

		if(light.value != "") {
			//Parse colour value

			stringstream col(light.value);

			col >> colour.x;
			col >> colour.y;
			col >> colour.z;
		}
        
        string colorExpr = vec3string(colour);
        
        float range = 20.0;
        float falloff = 2.0;
        
		SUPoint3D pos = {0};
		pos = (pos * light.transform) / g_Scale;
        string posExpr = vec3string(pos);
        
        
        if(light.attributes["range"] != "") {
            range = (float)atof(light.attributes["range"].c_str());
        }
        
        if(light.attributes["falloff"] != "") {
            falloff = (float)atof(light.attributes["falloff"].c_str());
        }


		if(light.type == "ambient") {
			m_Shader << "ambientLight = " << colorExpr << ";";
			if(m_Debug) m_Shader << endl;
		}

		if(light.type == "spotlight") {

			float outerCone = 40;
            if(light.attributes["outerCone"] != "") {
                outerCone = (float)atoi(light.attributes["outerCone"].c_str());
            }
            
			float innerCone = outerCone - 0.5;
            if(light.attributes["innerCone"] != "") {
                outerCone = (float)atoi(light.attributes["innerCone"].c_str());
            }
            
			SUVector3D dir = yaxis * light.transform;
            
			string func = "spotlight";
			if(light.attributes["func"] != "") {
				func = light.attributes["func"];
			}
            
			if(light.attributes["cond"] != "") {
				string cond = stringReplace(light.attributes["cond"],"$POS",posExpr);
                m_Shader << "if(" << cond << ") ";
			}

			m_Shader << func << "(" << posExpr << "," << vec3string(dir) << "," << colorExpr<< ")," << outerCone << "," << innerCone << "," << range << "," << falloff << ");";
			if(m_Debug) m_Shader << endl;
		}

		if(light.type == "light") {
				
			string func = "pointlight";
			if(light.attributes["func"] != "") {
				func = light.attributes["func"];
			}
            
			if(light.attributes["cond"] != "") {
				string cond = stringReplace(light.attributes["cond"],"$POS",posExpr);
                m_Shader << "if(" << cond << ") ";
			}

			m_Shader << func << "(" << posExpr << "," << colour.x << "," << colour.y << "," << colour.z << "),0.00,"<<range<<"," << falloff << ");";
			if(m_Debug) m_Shader << endl;
		}

	}
	
	m_Shader << "}";
}
