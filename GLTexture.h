#pragma once

class GLTexture
{
public:
	GLTexture() {}
	~GLTexture();

	void Make(int w, int h, const char* texData);
	void SetMode(const int mode);

	void Bind();
	void Unbind();
	inline unsigned int TexNum() { return n; }

private:
	unsigned int n = -1;
};

