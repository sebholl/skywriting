package skywriting.examples.terasort;
/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


import java.io.IOException;
import java.io.InputStream;

import skywriting.examples.grep.Text;

/**
 * Treats keys as offset in file and value as line. 
 * @deprecated Use 
 *   {@link org.apache.hadoop.mapreduce.lib.input.LineRecordReader} instead.
 */
@Deprecated
public class LineRecordReader implements RecordReader<LongWritable, Text> {
  /*private static final Log LOG
    = LogFactory.getLog(LineRecordReader.class.getName());*/

  //private CompressionCodecFactory compressionCodecs = null;
  private long start;
  private long pos;
  private long end;
  private LineReader in;
  int maxLineLength;

  /**
   * A class that provides a line reader from an input stream.
   * @deprecated Use {@link org.apache.hadoop.util.LineReader} instead.
   */
  @Deprecated
  public static class InnerLineReader extends LineReader {
    InnerLineReader(InputStream in) {
      super(in);
    }
    InnerLineReader(InputStream in, int bufferSize) {
      super(in, bufferSize);
    }
/*    public InnerLineReader(InputStream in, Configuration conf) throws IOException {
      super(in, conf);
    }*/
  }

  public LineRecordReader(InputStream fileIn, int start, int end) throws IOException {
    this.maxLineLength = Integer.MAX_VALUE; /*job.getInt("mapred.linerecordreader.maxlength",
                                    Integer.MAX_VALUE);*/
    this.start = start; //split.getStart();
    this.end = end;
    //final Path file = split.getPath();
    //compressionCodecs = new CompressionCodecFactory(job);
    //final CompressionCodec codec = compressionCodecs.getCodec(file);

    // open the file and seek to the start of the split
    //FileSystem fs = file.getFileSystem(job);
    //FSDataInputStream fileIn = fs.open(split.getPath());
    boolean skipFirstLine = false;
    //if (codec != null) {
//      in = new LineReader(codec.createInputStream(fileIn), job);
  //    end = Long.MAX_VALUE;
//    } else {
      if (start != 0) {
        skipFirstLine = true;
        --(this.start);
        fileIn.skip(start);
        //fileIn.seek(start);
      }
      in = new LineReader(fileIn);
  //  }
    if (skipFirstLine) {  // skip first line and re-establish "start".
      this.start += in.readLine(new Text(), 0,
                           (int)Math.min((long)Integer.MAX_VALUE, end - start));
    }
    this.pos = this.start;
  }

  /*
  public LineRecordReader(InputStream in, long offset, long endOffset,
                          int maxLineLength) throws IOException {
    this.maxLineLength = maxLineLength;
    this.in = new LineReader(in);
    this.start = offset;
    this.pos = offset;
    this.end = endOffset;
    
  }
  */
  
/*
  public LineRecordReader(InputStream in, long offset, long endOffset, 
                          Configuration job) 
    throws IOException{
    this.maxLineLength = job.getInt("mapred.linerecordreader.maxlength",
                                    Integer.MAX_VALUE);
    this.in = new LineReader(in, job);
    this.start = offset;
    this.pos = offset;
    this.end = endOffset;    
  }
  */
  public LongWritable createKey() {
    return new LongWritable();
  }
  
  public Text createValue() {
    return new Text();
  }
  
  /** Read a line. */
  public synchronized boolean next(LongWritable key, Text value)
    throws IOException {

    while (pos < end) {
      key.set(pos);

      int newSize = in.readLine(value, maxLineLength,
                                Math.max((int)Math.min(Integer.MAX_VALUE, end-pos),
                                         maxLineLength));
      if (newSize == 0) {
        return false;
      }
      pos += newSize;
      if (newSize < maxLineLength) {
        return true;
      }

      // line too long. try again
      //LOG.info("Skipped line of size " + newSize + " at pos " + (pos - newSize));
    }

    return false;
  }

  /**
   * Get the progress within the split
   */
  public float getProgress() {
    if (start == end) {
      return 0.0f;
    } else {
      return Math.min(1.0f, (pos - start) / (float)(end - start));
    }
  }
  
  public  synchronized long getPos() throws IOException {
    return pos;
  }

  public synchronized void close() throws IOException {
    if (in != null) {
      in.close(); 
    }
  }
}
