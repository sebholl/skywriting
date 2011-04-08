/*
 * Copyright (c) 2011 Derek Murray <Derek.Murray@cl.cam.ac.uk>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
package com.asgow.ciel.references;

import com.asgow.ciel.protocol.CielProtos.Reference.Builder;
import com.asgow.ciel.protocol.CielProtos.Reference.ReferenceType;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;

/**
 * @author dgm36
 *
 */
public final class FutureReference extends Reference {

	public FutureReference(String id) {
		super(id);
	}
	
	public FutureReference(com.asgow.ciel.protocol.CielProtos.Reference ref) {
		super(ref);
	}

	public FutureReference(JsonArray refTuple) {
		super(refTuple.get(1).getAsString());
	}
	
	@Override
	public boolean isConsumable() {
		return false;
	}

	@Override
	public Builder buildProtoBuf(Builder builder) {
		return builder.setType(ReferenceType.FUTURE);
	}

	public static JsonPrimitive IDENTIFIER = new JsonPrimitive("f2");
	@Override
	public JsonObject toJson() {
		JsonArray ret = new JsonArray();
		ret.add(IDENTIFIER);
		ret.add(new JsonPrimitive(this.getId()));
		return Reference.wrapAsReference(ret);
	}
	
	public String toString() {
		return "FutureReference(" + this.getId() + ")";
	}
	
	

	
}
