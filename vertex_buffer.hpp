//
// Created by Saliya Ekanayake on 7/9/17.
//

#ifndef CLIONCPP_VERTEX_BUFFER_HPP
#define CLIONCPP_VERTEX_BUFFER_HPP

#endif //CLIONCPP_VERTEX_BUFFER_HPP

class vertex_buffer{
public:
  vertex_buffer(){};
  vertex_buffer(int vertex_offset_factor, int buffer_offset_factor, std::shared_ptr<short> buffer){
    this->vertex_offset_factor = vertex_offset_factor;
    this->buffer_offset_factor = buffer_offset_factor;
    this->buffer = buffer;
  }
  virtual ~vertex_buffer(){}

  int get_vertex_offset_factor() const {
    return vertex_offset_factor;
  }

  int get_buffer_offset_factor() const {
    return buffer_offset_factor;
  }

  void set_vertex_offset_factor(int vertex_offset_factor) {
    this->vertex_offset_factor = vertex_offset_factor;
  }

  void set_buffer(std::shared_ptr<short> buffer, int buffer_offset_factor) {
    this->buffer = buffer;
    this->buffer_offset_factor = buffer_offset_factor;
  }

  void set_buffer(std::shared_ptr<short> buffer) {
    this->buffer = buffer;
  }

  std::shared_ptr<short> get_buffer(){
    return buffer;
  }

private:
  int vertex_offset_factor;
  int buffer_offset_factor;

protected:
  std::shared_ptr<short> buffer;
};

